#!/usr/bin/env python3
"""
RISC-V emulator test harness.
Assembles .s files, runs emulator, compares dump output against expected values.

Usage: python3 run_tests.py [options] [test_dir] [emulator_path]
  test_dir:      directory containing .s + .json test pairs (default: test/)
  emulator_path: path to emulator binary (default: build/riscv_emu)

Options:
  -j, --jobs N      number of parallel workers (default: CPU count)
  -k, --filter PAT  only run tests whose stem contains PAT (substring match)
  -x, --fail-fast   stop scheduling new tests after the first failure
  -v, --verbose     show emulator stdout/stderr for failing tests
  -t, --timeout S   per-test emulator timeout in seconds (default: 10)
  --no-color        disable colored output

Test format:
  Each test is a pair of files with the same stem:
    foo.s    - assembly source
    foo.json - expected values to check

  JSON format:
  {
    "name": "test name",
    "pc": "0x7C",
    "regs": {
      "x01": "0x1200",
      "x05": "0x10"
    },
    "mem": {
      "0x1000": "f28a7b15",
      "0x11f8": "b863b9c8"
    }
  }

  All fields optional. Only specified values are checked.
  Register values are full 64-bit hex.
  Memory values are 4-byte little-endian words (as displayed in dump).
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

GCC = "riscv64-unknown-elf-gcc"
OBJCOPY = "riscv64-unknown-elf-objcopy"
ASM_MARCH_OPT = "rv64im"


# ---------------------------------------------------------------------------
# Output helpers
# ---------------------------------------------------------------------------

class Color:
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
            f.write("SECTIONS { . = 0x0; .text : { *(.text) } }\n")

        result = subprocess.run(
            [GCC, "-nostdlib", f"-march={ASM_MARCH_OPT}", "-mabi=lp64",
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


def read_mem_word(mem, addr):
    """Read a 4-byte little-endian word from parsed memory.
    Returns None if no byte in the word appears in the dump at all."""
    parts = [mem.get(addr + i) for i in range(4)]
    if all(p is None for p in parts):
        return None
    b0, b1, b2, b3 = (p or 0 for p in parts)
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
        # Mem values are documented as raw hex words ("f28a7b15"), no 0x prefix.
        exp_val = int(exp_val_str, 16)
        actual = read_mem_word(mem, addr)
        if actual is None:
            failures.append(f"mem[0x{addr:x}] not present in dump")
        elif actual != exp_val:
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
    tests = []
    skipped = []
    for f in sorted(os.listdir(test_dir)):
        if not f.endswith(".json"):
            continue
        stem = f[:-5]
        if pattern and pattern not in stem:
            continue
        s_file = os.path.join(test_dir, stem + ".s")
        if os.path.isfile(s_file):
            tests.append((s_file, os.path.join(test_dir, f)))
        else:
            skipped.append(stem)
    return tests, skipped


def print_result(r, width):
    status = green("PASS") if r.passed else red("FAIL")
    print(f"  {status}  {r.name:<{width}} {dim(f'{r.duration:6.2f}s')}")
    for line in r.log:
        print(line)


def main():
    ap = argparse.ArgumentParser(description="RISC-V emulator test harness")
    ap.add_argument("test_dir", nargs="?", default="test",
                    help="directory containing .s + .json test pairs")
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

    if not os.path.isdir(args.test_dir):
        print(red(f"Test directory not found at {args.test_dir}"))
        sys.exit(1)

    check_toolchain()

    tests, skipped = collect_tests(args.test_dir, args.filter)

    for stem in skipped:
        print(yellow(f"  SKIP: {stem} — no matching .s file"))

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