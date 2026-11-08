#!/usr/bin/env python3
"""
RISC-V emulator test harness.
Assembles .s files, runs emulator, compares dump output against expected values.

Usage: python3 run_tests.py [options] [test_dir] [emulator_path]
  test_dir:      directory holding programs/ and expected/ (default: this script's folder)
  emulator_path: path to emulator binary (default: build/riscv_emu)

Layout:
  test_dir/
    programs/foo.s       - assembly source
    expected/foo.json    - expected values for that program

Programs are linked at the RAM base (0x80000000), where the emulator loads
them and where the hart starts executing. End each program by writing the
clean-shutdown magic 0x5555 to the shutdown register at 0x00100000
(sifive_test); the emulator then stops, dumps machine state and exits. Do
not assert registers clobbered by the shutdown write (t3/t4 are used here).

Toolchain:
  RISCV_PREFIX  tool prefix (default: riscv64-unknown-elf-)

Options:
  -j, --jobs N      number of parallel workers (default: CPU count)
  -k, --filter PAT  only run tests whose stem contains PAT (substring match)
  -x, --fail-fast   stop scheduling new tests after the first failure
  -v, --verbose     show emulator stdout/stderr for failing tests
  -t, --timeout S   per-test emulator timeout in seconds (default: 10)
  --no-color        disable colored output

JSON format:
  {
    "name": "test name",
    "pc": "0x8000007C",
    "regs": {
      "x01": "0x1200",
      "x05": "-1"
    },
    "mem": {
      "0x80010000": "f28a7b15"
    }
  }

  All fields optional. Only specified values are checked.
  Register values: hex with 0x prefix, or signed decimal; compared as 64-bit.
  Memory keys are physical addresses inside RAM (>= 0x80000000).
  Memory values are 4-byte little-endian words, hex without 0x prefix.
"""

import argparse
import json
import os
import shutil
import subprocess
import sys
import tempfile
import time
from concurrent.futures import ThreadPoolExecutor, as_completed

RISCV_PREFIX = os.environ.get("RISCV_PREFIX", "riscv64-unknown-elf-")
GCC = RISCV_PREFIX + "gcc"
OBJCOPY = RISCV_PREFIX + "objcopy"
ASM_MARCH_OPT = "rv64ima_zicsr_zifencei"

# Must match the emulator: RAM is mapped here, programs are loaded at RAM
# offset 0, and the hart's reset PC points here.
RAM_BASE = 0x80000000

# .text first so the entry point (_start) is the first instruction at RAM_BASE.
LINKER_SCRIPT = f"""\
ENTRY(_start)
SECTIONS
{{
    . = 0x{RAM_BASE:X};
    .text : {{ *(.text.start) *(.text*) }}
    .rodata : {{ *(.rodata*) }}
    .data : {{ *(.data*) *(.sdata*) }}
    .bss : {{ *(.bss*) *(.sbss*) }}
}}
"""

PROGRAMS_DIR = "programs"
EXPECTED_DIR = "expected"


# ---------------------------------------------------------------------------
# Output helpers
# ---------------------------------------------------------------------------

class Color:
    def __init__(self):
        pass

    enabled = sys.stdout.isatty() and os.environ.get("NO_COLOR") is None

    GREEN = "\033[32m"
    RED = "\033[31m"
    YELLOW = "\033[33m"
    DIM = "\033[2m"
    BOLD = "\033[1m"
    RESET = "\033[0m"

    @classmethod
    def wrap(cls, code, text):
        if not cls.enabled:
            return text
        return f"{code}{text}{cls.RESET}"


def green(s): return Color.wrap(Color.GREEN, s)


def red(s): return Color.wrap(Color.RED, s)


def yellow(s): return Color.wrap(Color.YELLOW, s)


def dim(s): return Color.wrap(Color.DIM, s)


def bold(s): return Color.wrap(Color.BOLD, s)


# ---------------------------------------------------------------------------
# Toolchain / assembly
# ---------------------------------------------------------------------------

def check_toolchain():
    missing = [t for t in (GCC, OBJCOPY) if shutil.which(t) is None]
    if missing:
        for tool in missing:
            print(red(f"Error: {tool} not found in PATH"))
        sys.exit(1)


def assemble(s_path, bin_path, log):
    """Assemble .s to .bin via gcc + objcopy. Returns True on success."""
    elf_path = bin_path.replace(".bin", ".elf")
    ld_path = bin_path.replace(".bin", ".ld")
    try:
        with open(ld_path, "w") as f:
            f.write(LINKER_SCRIPT)

        # -mcmodel=medany: address symbols PC-relatively, so code linked at
        #   0x80000000 works (the default medlow only reaches the low 2 GiB).
        # -static / -no-pie / --build-id=none: plain bare-metal image, also when
        #   using a Linux-targeted cross compiler.
        result = subprocess.run(
            [GCC, "-nostdlib", "-static", "-no-pie", "-Wl,--build-id=none",
             f"-march={ASM_MARCH_OPT}", "-mabi=lp64", "-mcmodel=medany",
             "-T", ld_path, "-o", elf_path, s_path],
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            log.append(red(f"  ASM ERROR: {os.path.basename(s_path)}"))
            log.append(dim(result.stderr.strip()))
            return False

        result = subprocess.run(
            [OBJCOPY, "-O", "binary", elf_path, bin_path],
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            log.append(red(f"  OBJCOPY ERROR: {os.path.basename(s_path)}"))
            log.append(dim(result.stderr.strip()))
            return False

        return True
    finally:
        for p in (elf_path, ld_path):
            if os.path.exists(p):
                os.remove(p)


# ---------------------------------------------------------------------------
# Dump parsing
# ---------------------------------------------------------------------------

def parse_dump(output):
    regs = {}
    mem = {}
    pc = None
    section = None

    for line in output.splitlines():
        line = line.strip()

        if line == "[HARTS]":
            section = "harts"
            continue
        elif line == "[RAM]":
            section = "ram"
            continue

        try:
            if section == "harts":
                if line.startswith("PC:"):
                    pc = int(line.split(":")[1].strip(), 16)
                elif line.startswith("x") and ":" in line:
                    reg, val = line.split(":", 1)
                    regs[reg.strip()] = int(val.strip(), 16)

            elif section == "ram":
                if ":" not in line:
                    continue
                addr_str, data_str = line.split(":", 1)
                addr = int(addr_str.strip(), 16)
                for i, b in enumerate(data_str.strip().split()):
                    mem[addr + i] = int(b, 16)
        except ValueError:
            # Malformed line in dump — skip rather than crash the harness.
            continue

    return pc, regs, mem


def read_mem_word(mem, offset):
    """Read a 4-byte little-endian word at a RAM offset from parsed memory.
    The dump omits all-zero rows, so bytes missing from it are zero."""
    b0, b1, b2, b3 = (mem.get(offset + i, 0) for i in range(4))
    return (b3 << 24) | (b2 << 16) | (b1 << 8) | b0


def parse_val(s):
    """Parse a value string as hex (0x prefix) or signed decimal, return as unsigned 64-bit."""
    if s.startswith("0x") or s.startswith("0X"):
        return int(s, 16)
    return int(s) & 0xFFFFFFFFFFFFFFFF


def normalize_reg(reg):
    reg = reg.lower()
    if reg.startswith("x"):
        try:
            return f"x{int(reg[1:]):02d}"
        except ValueError:
            pass
    return reg


# ---------------------------------------------------------------------------
# Test execution
# ---------------------------------------------------------------------------

class TestResult:
    def __init__(self, name, passed, duration, log):
        self.name = name
        self.passed = passed
        self.duration = duration
        self.log = log


def run_test(s_path, json_path, emu_path, tmp_dir, timeout, verbose):
    """Run a single test. Returns a TestResult; all output is buffered in
    result.log so parallel workers don't interleave their printing."""
    log = []
    start = time.monotonic()

    def done(passed):
        return TestResult(name, passed, time.monotonic() - start, log)

    stem = os.path.splitext(os.path.basename(s_path))[0]
    name = stem

    try:
        with open(json_path, "r") as f:
            expected = json.load(f)
    except (json.JSONDecodeError, OSError) as e:
        log.append(red(f"  BAD JSON: {os.path.basename(json_path)} — {e}"))
        return done(False)

    name = expected.get("name", stem)
    bin_path = os.path.join(tmp_dir, stem + ".bin")

    if not assemble(s_path, bin_path, log):
        return done(False)

    try:
        result = subprocess.run(
            [emu_path, bin_path],
            capture_output=True,
            text=True,
            timeout=timeout,
        )
        output = result.stdout
    except subprocess.TimeoutExpired:
        log.append(red(f"  TIMEOUT ({timeout}s)"))
        return done(False)

    if result.returncode != 0:
        log.append(yellow(f"  warning: emulator exited with code {result.returncode}"))
        if verbose and result.stderr.strip():
            log.append(dim(result.stderr.strip()))

    if not output.strip():
        log.append(red("  no output from emulator"))
        return done(False)

    pc, regs, mem = parse_dump(output)
    failures = []

    if expected.get("mem") and "[RAM]" not in output:
        failures.append("no [RAM] section in dump")

    if "pc" in expected:
        exp_pc = parse_val(expected["pc"])
        if pc is None:
            failures.append("PC not found in dump")
        elif pc != exp_pc:
            failures.append(f"PC expected 0x{exp_pc:x}, got 0x{pc:x}")

    for reg, exp_val_str in expected.get("regs", {}).items():
        exp_val = parse_val(exp_val_str)
        reg_key = normalize_reg(reg)
        actual = regs.get(reg_key)
        if actual is None:
            failures.append(f"{reg_key} not found in dump")
        elif actual != exp_val:
            failures.append(
                f"{reg_key} expected 0x{exp_val:016x}, got 0x{actual:016x}")

    for addr_str, exp_val_str in expected.get("mem", {}).items():
        addr = int(addr_str, 16)
        if addr < RAM_BASE:
            failures.append(
                f"mem[0x{addr:x}] is below RAM (0x{RAM_BASE:x}); use physical addresses")
            continue
        # Mem values are raw hex words ("f28a7b15"), no 0x prefix.
        exp_val = int(exp_val_str, 16)
        # The RAM dump is indexed by offset from the start of RAM.
        actual = read_mem_word(mem, addr - RAM_BASE)
        if actual != exp_val:
            failures.append(
                f"mem[0x{addr:x}] expected 0x{exp_val:08x}, got 0x{actual:08x}")

    for f_msg in failures:
        log.append(red(f"    {f_msg}"))

    if failures and verbose:
        log.append(dim("  --- emulator output ---"))
        log.append(dim(output.rstrip()))
        log.append(dim("  -----------------------"))

    return done(not failures)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def collect_tests(test_dir, pattern):
    """Pair programs/<stem>.s with expected/<stem>.json.
    Returns (tests, skipped) where skipped lists (stem, reason)."""
    programs = os.path.join(test_dir, PROGRAMS_DIR)
    expected = os.path.join(test_dir, EXPECTED_DIR)

    stems_s = {f[:-2] for f in os.listdir(programs) if f.endswith(".s")}
    stems_j = {f[:-5] for f in os.listdir(expected) if f.endswith(".json")}

    tests = []
    skipped = []
    for stem in sorted(stems_s | stems_j):
        if pattern and pattern not in stem:
            continue
        if stem not in stems_s:
            skipped.append((stem, f"no {PROGRAMS_DIR}/{stem}.s"))
        elif stem not in stems_j:
            skipped.append((stem, f"no {EXPECTED_DIR}/{stem}.json"))
        else:
            tests.append((os.path.join(programs, stem + ".s"),
                          os.path.join(expected, stem + ".json")))
    return tests, skipped


def print_result(r, width):
    status = green("PASS") if r.passed else red("FAIL")
    print(f"  {status}  {r.name:<{width}} {dim(f'{r.duration:6.2f}s')}")
    for line in r.log:
        print(line)


def main():
    ap = argparse.ArgumentParser(description="RISC-V emulator test harness")
    ap.add_argument("test_dir", nargs="?",
                    default=os.path.dirname(os.path.abspath(__file__)),
                    help="directory holding programs/ and expected/")
    ap.add_argument("emu_path", nargs="?", default="build/riscv_emu",
                    help="path to emulator binary")
    ap.add_argument("-j", "--jobs", type=int, default=os.cpu_count() or 1,
                    help="number of parallel workers")
    ap.add_argument("-k", "--filter", default=None, metavar="PAT",
                    help="only run tests whose stem contains PAT")
    ap.add_argument("-x", "--fail-fast", action="store_true",
                    help="stop scheduling new tests after first failure")
    ap.add_argument("-v", "--verbose", action="store_true",
                    help="show emulator output for failing tests")
    ap.add_argument("-t", "--timeout", type=float, default=10.0,
                    help="per-test emulator timeout in seconds")
    ap.add_argument("--no-color", action="store_true",
                    help="disable colored output")
    args = ap.parse_args()

    if args.no_color:
        Color.enabled = False

    if not os.path.isfile(args.emu_path):
        print(red(f"Emulator not found at {args.emu_path}"))
        sys.exit(1)

    for sub in (PROGRAMS_DIR, EXPECTED_DIR):
        path = os.path.join(args.test_dir, sub)
        if not os.path.isdir(path):
            print(red(f"Test directory not found at {path}"))
            sys.exit(1)

    check_toolchain()

    tests, skipped = collect_tests(args.test_dir, args.filter)

    for stem, reason in skipped:
        print(yellow(f"  SKIP: {stem} — {reason}"))

    if not tests:
        print("No tests found.")
        sys.exit(1)

    jobs = max(1, min(args.jobs, len(tests)))
    print(bold(f"Running {len(tests)} test(s) with {jobs} worker(s):\n"))

    name_width = max(
        len(os.path.splitext(os.path.basename(s))[0]) for s, _ in tests)
    name_width = min(max(name_width, 12), 48)

    results = []
    start = time.monotonic()
    stop = False

    with tempfile.TemporaryDirectory() as tmp_dir:
        with ThreadPoolExecutor(max_workers=jobs) as pool:
            futures = {}
            for s_path, json_path in tests:
                fut = pool.submit(run_test, s_path, json_path, args.emu_path,
                                  tmp_dir, args.timeout, args.verbose)
                futures[fut] = s_path

            for fut in as_completed(futures):
                try:
                    r = fut.result()
                except Exception as e:
                    stem = os.path.splitext(
                        os.path.basename(futures[fut]))[0]
                    r = TestResult(stem, False, 0.0,
                                   [red(f"    harness error: {e!r}")])
                results.append(r)
                print_result(r, name_width)
                if not r.passed and args.fail_fast and not stop:
                    stop = True
                    for other in futures:
                        other.cancel()

    total = time.monotonic() - start
    passed = sum(1 for r in results if r.passed)
    failed = len(results) - passed

    print()
    if failed:
        failed_names = ", ".join(r.name for r in results if not r.passed)
        print(red(f"Failed: {failed_names}"))
    summary = f"{passed} passed, {failed} failed in {total:.2f}s"
    print(bold(green(summary) if failed == 0 else red(summary)))
    sys.exit(0 if failed == 0 else 1)


if __name__ == "__main__":
    main()
