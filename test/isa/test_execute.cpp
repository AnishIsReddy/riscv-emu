// test_execute.cpp
// ---------------------------------------------------------------------------
// Build (link against your own decode.cpp / execute.cpp):
//
//     g++ -std=c++20 -O2 -Wall -Wextra test_execute.cpp decode.cpp execute.cpp -o test_execute
//
// Run:
//     ./test_execute                 # run everything
//     ./test_execute --list          # print test names, one per line
//     ./test_execute --filter=amo    # run tests whose name contains "amo"
//     ./test_execute --list --filter=load
//
// Exit code: 0 if every selected test passed, 1 otherwise. Per-test lines are
// "PASS: <name>" / "FAIL: <name>" (details indented on following lines) and a
// final "SUMMARY: <p>/<n> passed". A Python runner can shell out, scan the
// PASS:/FAIL: lines, and fold results into the same table as your .s tests.
//
// Assembler selection (no library dependency; only a build-time toolchain):
//   RISCV_PREFIX   e.g. "riscv64-linux-gnu-"  (auto-detected if unset)
//   RISCV_MARCH    default "rv64ima_zicsr_zifencei"
// ---------------------------------------------------------------------------

#include <algorithm>

#include "../../src/decode.h"
#include "../../src/defs.h"
#include "../../src/execute.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <expected>
#include <format>
#include <functional>
#include <iostream>
#include <meta>
#include <print>
#include <ranges>
#include <source_location>
#include <sstream>
#include <string>
#include <type_traits>
#include <unistd.h>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

using namespace riscv_emu;

// ===========================================================================
// Failure type + small formatting helpers
// ===========================================================================
namespace {
struct TestFailure : std::exception
{
    std::string msg;
    explicit TestFailure(std::string m) : msg{std::move(m)} {}
    [[nodiscard]] const char* what() const noexcept override { return msg.c_str(); }
};
} // namespace

static std::string loc(const std::source_location s = std::source_location::current())
{
    return std::format("{}:{}: ", s.file_name(), s.line());
}

template <class E>
    requires std::is_enum_v<E>
static std::string_view enum_name(const E value)
{
    template for (constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^E)))
    {
        if (value == [:e:])
            return std::meta::identifier_of(e); // fine in C++26
    }
    return "<unnamed>";
}

// Pretty-print a scalar as hex (+decimal), so got/want lines are easy to read.
template <class T>
static std::string show(const T& v)
{
    if constexpr (std::is_same_v<std::remove_cv_t<T>, bool>) {
        return v ? "true" : "false";
    }
    else if constexpr (std::is_enum_v<T>) {
        return std::format("{}::{} ({})", std::meta::identifier_of(^^T), enum_name(v), std::to_underlying(v));
    }
    else if constexpr (std::is_integral_v<T>) {
        // Sign-extend to 64-bit first so negatives show as full-width
        // two's complement (matches the old behavior; handy for RV64 values).
        const auto u = static_cast<unsigned long long>(static_cast<long long>(v));
        return std::format("{:#x} ({})", u, static_cast<long long>(v));
    }
    else if constexpr (std::formattable<T, char>) {
        return std::format("{}", v);
    }
    else {
        // last-ditch fallback for stream-only types
        std::ostringstream o;
        o << v;
        return o.str();
    }
}

// Name the variant alternatives, in their declaration order in instr_effect.
static std::string_view effect_name(const instr_effect& eff)
{
    return eff.effect.visit(
        []<class T>(const T&)
        {
            constexpr auto name = std::meta::display_string_of(^^T); // evaluated at compile time, per alternative
            return name;
        });
}

// Indent any continuation lines so multi-line failure messages stay readable.
static std::string indent(const std::string& s)
{
    return s |
           std::views::split('\n') |
           std::views::join_with(std::string_view{"\n      "}) |
           std::ranges::to<std::string>();
}

// ===========================================================================
// Assertions (all throw TestFailure -> reported cleanly, never crash)
// ===========================================================================

// Safely pull a specific alternative out of an effect. On a mismatch it throws
// TestFailure (with the alternative we wanted vs. the one actually present)
// instead of letting std::get blow up with bad_variant_access.

template <class T>
static const T& expect_alt(const instr_effect& eff, const std::source_location s = std::source_location::current())
{
    if (const T* p = std::get_if<T>(&eff.effect)) {
        return *p;
    }
    constexpr auto want = std::meta::display_string_of(^^T);
    throw TestFailure(
        std::format("{}:{}: expected effect {}, but got {}", s.file_name(), s.line(), want, effect_name(eff)));
}

#define GET_ALT(EFF, T) expect_alt<T>(EFF, std::source_location::current())

#define CHECK_EQ(GOT, WANT)                                                                                            \
    do {                                                                                                               \
        auto _got = (GOT);                                                                                             \
        auto _want = (WANT);                                                                                           \
        if (!(_got == _want))                                                                                          \
            throw TestFailure(loc() + "CHECK_EQ(" #GOT ", " #WANT "): got=" + show(_got) + " want=" + show(_want));    \
    }                                                                                                                  \
    while (0)

#define CHECK_TRUE(COND)                                                                                               \
    do {                                                                                                               \
        if (!(COND))                                                                                                   \
            throw TestFailure(loc() + "CHECK_TRUE(" #COND ") failed");                                                 \
    }                                                                                                                  \
    while (0)

// ===========================================================================
// Test registry + self-registration
// ===========================================================================
namespace {
using TestCase = std::pair<const char*, void (*)()>;

std::vector<TestCase>& registry()
{
    static std::vector<TestCase> r;
    return r;
}


struct Registrar
{
    Registrar(const char* n, void (*f)()) { registry().emplace_back(n, f); }
};
} // namespace

#define TEST(NAME)                                                                                                     \
    static void NAME();                                                                                                \
    static Registrar reg_##NAME(#NAME, &NAME);                                                                         \
    static void NAME()

// ===========================================================================
// Assembler: turn one asm line into a 32-bit instruction word.
// Only a build-time dependency (binutils); cached per asm string.
// ===========================================================================
namespace assembler {
namespace {
struct ShellResult
{
    int code;
    std::string out;
};
} // namespace

static ShellResult run_shell(const std::string& cmd)
{
    FILE* p = popen((cmd + " 2>&1").c_str(), "r");
    if (!p) {
        return {.code = -1, .out = "popen failed"};
    }
    std::string out;
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, p)) > 0) {
        out.append(buf, n);
    }
    const int status = pclose(p);
    const int code = (status == -1) ? -1 : (WIFEXITED(status) ? WEXITSTATUS(status) : -1);
    return {.code = code, .out = out};
}

static std::string march()
{
    if (const char* e = std::getenv("RISCV_MARCH")) {
        return e;
    }
    return "rv64ima_zicsr_zifencei";
}

static std::vector<std::string> candidate_prefixes()
{
    std::vector<std::string> v;
    if (const char* e = std::getenv("RISCV_PREFIX"))
        v.emplace_back(e);
    v.insert(v.end(), {
                          "riscv64-linux-gnu-",
                          "riscv64-unknown-linux-gnu-",
                          "riscv64-unknown-elf-",
                          "riscv-none-elf-",
                          "riscv64-elf-",
                      });
    return v;
}

// Resolved-once working prefix.
static std::string& cached_prefix()
{
    static std::string p;
    return p;
}

static bool& prefix_known()
{
    static bool b = false;
    return b;
}

static std::expected<uint32_t, std::string> assemble_uncached(const std::string& line)
{
    char tmpl[] = "/tmp/rvtest_XXXXXX";
    const int fd = mkstemp(tmpl);
    if (fd < 0) {
        return std::unexpected("mkstemp failed");
    }
    // ".option norvc" forbids a 2-byte compressed form even if march enables 'c'.
    // ".option norelax" stops branch/jump relaxation (which can expand one
    // instruction into a multi-instruction veneer).
    // "__here:" is an anchor label: write PC-relative branch/jump targets as
    // e.g. "beq x1, x2, __here + 8" so the offset is encoded directly instead
    // of being treated as an absolute address that needs relocation.
    const std::string src = ".option norvc\n.option norelax\n.text\n__here:\n" + line + "\n";
    if (write(fd, src.data(), src.size()) < 0) {
        close(fd);
        return std::unexpected("write failed");
    }
    close(fd);

    const std::string s = tmpl;
    const std::string obj = s + ".o";
    const std::string bin = s + ".bin";
    auto cleanup = [&]
    {
        unlink(s.c_str());
        unlink(obj.c_str());
        unlink(bin.c_str());
    };

    const std::vector<std::string> prefixes = prefix_known() ? std::vector{cached_prefix()} : candidate_prefixes();

    std::string tried;
    for (const auto& pre : prefixes) {
        auto [code1, out1] = run_shell(std::format("{}as -march={} -o {} {}", pre, march(), obj, s));
        if (code1 != 0) {
            tried += std::format("  [{}as] {}", pre, out1);
            continue;
        }

        auto [code2, out2] = run_shell(std::format("{}objcopy -O binary -j .text {} {}", pre, obj, bin));
        if (code2 != 0) {
            tried += std::format("  [{}objcopy] {}", pre, out2);
            continue;
        }

        FILE* f = fopen(bin.c_str(), "rb");
        if (!f) {
            tried += std::format("  [open {}] failed\n", bin);
            continue;
        }

        unsigned char b[8];
        const size_t n = fread(b, 1, sizeof b, f);
        fclose(f);
        if (n != 4) {
            cleanup();
            auto out = std::format("assembling '{}' produced {} bytes "
                                   "(expected 4 -- pseudo-instruction expansion or wrong width?)",
                                   line, std::to_string(n));
            return std::unexpected(out);
        }
        cached_prefix() = pre;
        prefix_known() = true;
        cleanup();
        return static_cast<uint32_t>(b[0]) |
               (static_cast<uint32_t>(b[1]) << 8) |
               (static_cast<uint32_t>(b[2]) << 16) |
               (static_cast<uint32_t>(b[3]) << 24);
    }
    cleanup();
    return std::unexpected("no working RISC-V assembler found (set RISCV_PREFIX). Tried:\n" + tried);
}

// Public: throws TestFailure on toolchain/encoding errors so the test reports a
// clean FAIL with the assembler's message rather than aborting the run.
static uint32_t assemble(const std::string& line)
{
    static std::unordered_map<std::string, uint32_t> cache;
    if (const auto it = cache.find(line); it != cache.end())
        return it->second;
    auto res = assemble_uncached(line);
    if (!res.has_value()) {
        throw TestFailure{res.error()};
    }
    cache.emplace(line, res.value());
    return res.value();
}
} // namespace assembler

// ===========================================================================
// Fixture: holds the inputs execute() reads (registers, pc, privilege).
// ===========================================================================
namespace {
struct Fixture
{
    uint64_t regs[REG_COUNT] = {}; // x0..x31, x0 stays 0
    uint64_t pc = 0x8000'0000;
    privilege_level priv = privilege_level::M;

    [[nodiscard]]
    instr_effect run(const uint32_t word) const
    {
        return execute(decode(word), regs, pc, priv);
    }

    [[nodiscard]]
    instr_effect run(const std::string& asm_line) const
    {
        return run(assembler::assemble(asm_line));
    }
};
} // namespace

// ===========================================================================
// Tests
// ===========================================================================
//
// Expectations below follow the RISC-V spec. Register values are deliberately
// small/positive where a "W"-form or memory size could otherwise make full-vs.
// truncated reads ambiguous, so the assertion holds for any correct design.
//


// Harness smoke test: validates the assembler integration independently of execute().
TEST(harness_assembles_addi)
{
    CHECK_EQ(assembler::assemble("addi x1, x2, 5"), static_cast<uint32_t>(0x00510093));
}

//
// instruction_tests.cpp
//
// End-to-end tests (assemble -> decode -> execute) covering every instr_type
// declared in defs.h. Same harness/conventions as the provided examples:
//   - TEST(name) { ... }
//   - Fixture with .run("asm"), .pc, .regs[]
//   - GET_ALT(e, instr_effect::<alt>)  == std::get<...>(e.effect)
//   - CHECK_EQ(actual, expected)
//   - "__here" resolves to the current instruction address (for PC-relative ops)

// ===========================================================================
// LUI / AUIPC  ->  update_rd
// ===========================================================================

TEST(lui_loads_sign_extended_upper_immediate)
{
    Fixture f;
    const auto e = f.run("lui x5, 0xfffff"); // bit 31 set -> sign-extends to 64b
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(static_cast<int>(u.rd), 5);
    CHECK_EQ(u.value, static_cast<uint64_t>(0xFFFF'FFFF'FFFF'F000ULL));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(auipc_adds_immediate_to_pc)
{
    Fixture f;
    const auto e = f.run("auipc x5, 0x1"); // pc + (1 << 12)
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(static_cast<int>(u.rd), 5);
    CHECK_EQ(u.value, f.pc + 0x1000);
    CHECK_EQ(e.new_pc, f.pc + 4);
}

// ===========================================================================
// OP_IMM  ->  update_rd   (ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI)
// ===========================================================================

TEST(addi_adds_signed_immediate)
{
    Fixture f;
    f.regs[2] = 10;
    const auto e = f.run("addi x1, x2, 5");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(static_cast<int>(u.rd), 1);
    CHECK_EQ(u.value, static_cast<uint64_t>(15));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(addi_negative_immediate_is_sign_extended)
{
    Fixture f;
    f.regs[2] = 0;
    const auto e = f.run("addi x1, x2, -1"); // -1 sign-extends through full width
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(0xFFFF'FFFF'FFFF'FFFFULL));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(slti_signed_compare_true)
{
    Fixture f;
    f.regs[2] = static_cast<uint64_t>(-1); // signed -1 < 5
    const auto e = f.run("slti x1, x2, 5");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(1));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(sltiu_unsigned_compare_true)
{
    Fixture f;
    f.regs[2] = 3; // unsigned 3 < 5
    const auto e = f.run("sltiu x1, x2, 5");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(1));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(sltiu_treats_negative_immediate_as_large_unsigned)
{
    Fixture f;
    f.regs[2] = 0; // 0 < (uint64)-1  -> true
    const auto e = f.run("sltiu x1, x2, -1");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(1));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(xori_bitwise_xor)
{
    Fixture f;
    f.regs[2] = 0x0F;
    const auto e = f.run("xori x1, x2, 0xff");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(0xF0));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(ori_bitwise_or)
{
    Fixture f;
    f.regs[2] = 0x0F;
    const auto e = f.run("ori x1, x2, 0xf0");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(0xFF));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(andi_bitwise_and)
{
    Fixture f;
    f.regs[2] = 0xFF;
    const auto e = f.run("andi x1, x2, 0xf0");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(0xF0));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(slli_shifts_left)
{
    Fixture f;
    f.regs[2] = 1;
    const auto e = f.run("slli x1, x2, 4");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(16));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(srli_logical_shift_right)
{
    Fixture f;
    f.regs[2] = 0xFF0;
    const auto e = f.run("srli x1, x2, 4");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(0xFF));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(srai_arithmetic_shift_right_preserves_sign)
{
    Fixture f;
    f.regs[2] = 0xFFFF'FFFF'FFFF'FF00ULL; // negative
    const auto e = f.run("srai x1, x2, 4");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(0xFFFF'FFFF'FFFF'FFF0ULL));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

// ===========================================================================
// OP  ->  update_rd   (ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND)
// ===========================================================================

TEST(add_adds_registers)
{
    Fixture f;
    f.regs[2] = 10;
    f.regs[3] = 20;
    const auto e = f.run("add x1, x2, x3");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(30));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(sub_subtracts_registers)
{
    Fixture f;
    f.regs[2] = 20;
    f.regs[3] = 5;
    const auto e = f.run("sub x1, x2, x3");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(15));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(sll_masks_shift_amount_to_six_bits)
{
    Fixture f;
    f.regs[2] = 1;
    f.regs[3] = 68; // 68 & 0x3f == 4
    const auto e = f.run("sll x1, x2, x3");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(16));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(slt_signed_compare)
{
    Fixture f;
    f.regs[2] = static_cast<uint64_t>(-1);
    f.regs[3] = 0;
    const auto e = f.run("slt x1, x2, x3"); // -1 < 0 signed
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(1));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(sltu_unsigned_compare)
{
    Fixture f;
    f.regs[2] = static_cast<uint64_t>(-1); // huge unsigned
    f.regs[3] = 1;
    const auto e = f.run("sltu x1, x2, x3"); // huge < 1 -> false
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(0));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(xor_bitwise)
{
    Fixture f;
    f.regs[2] = 0xFF;
    f.regs[3] = 0x0F;
    const auto e = f.run("xor x1, x2, x3");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(0xF0));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(srl_logical_shift_right)
{
    Fixture f;
    f.regs[2] = 0xFF0;
    f.regs[3] = 4;
    const auto e = f.run("srl x1, x2, x3");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(0xFF));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(sra_arithmetic_shift_right)
{
    Fixture f;
    f.regs[2] = 0xFFFF'FFFF'FFFF'FF00ULL;
    f.regs[3] = 4;
    const auto e = f.run("sra x1, x2, x3");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(0xFFFF'FFFF'FFFF'FFF0ULL));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(or_bitwise)
{
    Fixture f;
    f.regs[2] = 0x0F;
    f.regs[3] = 0xF0;
    const auto e = f.run("or x1, x2, x3");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(0xFF));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(and_bitwise)
{
    Fixture f;
    f.regs[2] = 0xFF;
    f.regs[3] = 0x0F;
    const auto e = f.run("and x1, x2, x3");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(0x0F));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

// ===========================================================================
// JAL / JALR  ->  update_rd (link) + control flow
// ===========================================================================

TEST(jal_links_return_address_and_jumps)
{
    constexpr Fixture f;
    const auto e = f.run("jal x1, __here + 8"); // PC-relative offset 8
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(static_cast<int>(u.rd), 1);
    CHECK_EQ(u.value, f.pc + 4); // link = return address
    CHECK_EQ(e.new_pc, f.pc + 8);
}

TEST(jal_backward_offset)
{
    constexpr Fixture f;
    const auto e = f.run("jal x1, __here - 8");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(static_cast<int>(u.rd), 1);
    CHECK_EQ(u.value, f.pc + 4); // link
    CHECK_EQ(e.new_pc, f.pc - 8);
}

TEST(jalr_clears_low_bit_of_target)
{
    Fixture f;
    f.regs[2] = 0x1235; // odd base -> low bit must be cleared
    const auto e = f.run("jalr x1, x2, 4");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(static_cast<int>(u.rd), 1);
    CHECK_EQ(u.value, f.pc + 4);
    CHECK_EQ(e.new_pc, static_cast<uint64_t>((0x1235 + 4) & ~static_cast<uint64_t>(1))); // 0x1238
}

TEST(jalr_negative_immediate_and_clears_low_bit)
{
    Fixture f;
    f.regs[2] = 0x2000;
    const auto e = f.run("jalr x5, x2, -3"); // (0x2000 - 3) & ~1 = 0x1FFC
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(static_cast<int>(u.rd), 5);
    CHECK_EQ(u.value, f.pc + 4);
    CHECK_EQ(e.new_pc, static_cast<uint64_t>((0x2000 - 3) & ~static_cast<uint64_t>(1)));
}

// ===========================================================================
// BRANCH  ->  no_effect   (BNE, BLT, BGE, BLTU, BGEU; + signed/unsigned edges)
// ===========================================================================

TEST(beq_taken_sets_branch_target)
{
    Fixture f;
    f.regs[1] = 7;
    f.regs[2] = 7;
    const auto e = f.run("beq x1, x2, __here + 8");
    (void)GET_ALT(e, instr_effect::no_effect); // branches never write a register
    CHECK_EQ(e.new_pc, f.pc + 8);
}

TEST(beq_not_taken_falls_through)
{
    Fixture f;
    f.regs[1] = 7;
    f.regs[2] = 8;
    const auto e = f.run("beq x1, x2, __here + 8");
    (void)GET_ALT(e, instr_effect::no_effect);
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(bne_taken)
{
    Fixture f;
    f.regs[1] = 5;
    f.regs[2] = 6;
    const auto e = f.run("bne x1, x2, __here + 8");
    (void)GET_ALT(e, instr_effect::no_effect);
    CHECK_EQ(e.new_pc, f.pc + 8);
}

TEST(bne_not_taken)
{
    Fixture f;
    f.regs[1] = 5;
    f.regs[2] = 5;
    const auto e = f.run("bne x1, x2, __here + 8");
    (void)GET_ALT(e, instr_effect::no_effect);
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(blt_uses_signed_comparison)
{
    Fixture f;
    f.regs[1] = static_cast<uint64_t>(-1); // signed -1
    f.regs[2] = 1;
    const auto e = f.run("blt x1, x2, __here + 8"); // -1 < 1 signed -> taken
    (void)GET_ALT(e, instr_effect::no_effect);
    CHECK_EQ(e.new_pc, f.pc + 8);
}

TEST(bltu_uses_unsigned_comparison)
{
    Fixture f;
    f.regs[1] = static_cast<uint64_t>(-1); // huge unsigned
    f.regs[2] = 1;
    const auto e = f.run("bltu x1, x2, __here + 8"); // huge < 1 -> NOT taken
    (void)GET_ALT(e, instr_effect::no_effect);
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(bge_uses_signed_comparison)
{
    Fixture f;
    f.regs[1] = 1;
    f.regs[2] = static_cast<uint64_t>(-1);
    const auto e = f.run("bge x1, x2, __here + 8"); // 1 >= -1 signed -> taken
    (void)GET_ALT(e, instr_effect::no_effect);
    CHECK_EQ(e.new_pc, f.pc + 8);
}

TEST(bgeu_uses_unsigned_comparison)
{
    Fixture f;
    f.regs[1] = static_cast<uint64_t>(-1); // huge unsigned
    f.regs[2] = 1;
    const auto e = f.run("bgeu x1, x2, __here + 8"); // huge >= 1 -> taken
    (void)GET_ALT(e, instr_effect::no_effect);
    CHECK_EQ(e.new_pc, f.pc + 8);
}

TEST(beq_backward_negative_offset)
{
    Fixture f;
    f.regs[1] = 3;
    f.regs[2] = 3;
    const auto e = f.run("beq x1, x2, __here - 4");
    (void)GET_ALT(e, instr_effect::no_effect);
    CHECK_EQ(e.new_pc, f.pc - 4);
}

// ===========================================================================
// LOAD  ->  load_rd_from_mem   (LB, LH, LBU, LHU, LWU, LD)
// ===========================================================================

TEST(lw_emits_signed_word_load)
{
    Fixture f;
    f.regs[2] = 0x1000;
    const auto e = f.run("lw x1, 8(x2)");
    auto& l = GET_ALT(e, instr_effect::load_rd_from_mem<uint32_t>);
    CHECK_EQ(static_cast<int>(l.rd), 1);
    CHECK_EQ(l.addr, static_cast<uint64_t>(0x1008));
    CHECK_EQ(l.sign_ext, true);
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(lb_signed_byte_load)
{
    Fixture f;
    f.regs[2] = 0x1000;
    const auto e = f.run("lb x1, 8(x2)");
    auto& l = GET_ALT(e, instr_effect::load_rd_from_mem<uint8_t>);
    CHECK_EQ(static_cast<int>(l.rd), 1);
    CHECK_EQ(l.addr, static_cast<uint64_t>(0x1008));
    CHECK_EQ(l.sign_ext, true);
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(lh_signed_halfword_load)
{
    Fixture f;
    f.regs[2] = 0x1000;
    const auto e = f.run("lh x1, 8(x2)");
    auto& l = GET_ALT(e, instr_effect::load_rd_from_mem<uint16_t>);
    CHECK_EQ(l.sign_ext, true);
    CHECK_EQ(l.addr, static_cast<uint64_t>(0x1008));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(lbu_unsigned_byte_load)
{
    Fixture f;
    f.regs[2] = 0x1000;
    const auto e = f.run("lbu x1, 8(x2)");
    auto& l = GET_ALT(e, instr_effect::load_rd_from_mem<uint8_t>);
    CHECK_EQ(l.sign_ext, false);
    CHECK_EQ(l.addr, static_cast<uint64_t>(0x1008));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(lhu_unsigned_halfword_load)
{
    Fixture f;
    f.regs[2] = 0x1000;
    const auto e = f.run("lhu x1, 8(x2)");
    auto& l = GET_ALT(e, instr_effect::load_rd_from_mem<uint16_t>);
    CHECK_EQ(l.sign_ext, false);
    CHECK_EQ(l.addr, static_cast<uint64_t>(0x1008));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(lwu_unsigned_word_load)
{
    Fixture f;
    f.regs[2] = 0x1000;
    const auto e = f.run("lwu x1, 8(x2)");
    auto& l = GET_ALT(e, instr_effect::load_rd_from_mem<uint32_t>);
    CHECK_EQ(l.sign_ext, false);
    CHECK_EQ(l.addr, static_cast<uint64_t>(0x1008));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(ld_doubleword_load)
{
    Fixture f;
    f.regs[2] = 0x1000;
    const auto e = f.run("ld x1, 8(x2)");
    auto& l = GET_ALT(e, instr_effect::load_rd_from_mem<uint64_t>);
    CHECK_EQ(static_cast<int>(l.rd), 1);
    CHECK_EQ(l.addr, static_cast<uint64_t>(0x1008));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

// ===========================================================================
// STORE  ->  store_mem   (SB, SH, SW, SD)
// value uses operands that fit the store width, so it's unambiguous whether
// truncation happens in execute() or the memory layer.
// ===========================================================================

TEST(sb_byte_store)
{
    Fixture f;
    f.regs[2] = 0x1000;
    f.regs[3] = 0xAB;
    const auto e = f.run("sb x3, 8(x2)");
    auto& s = GET_ALT(e, instr_effect::store_mem<uint8_t>);
    CHECK_EQ(s.addr, static_cast<uint64_t>(0x1008));
    CHECK_EQ(s.value, static_cast<uint8_t>(0xAB));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(sh_halfword_store)
{
    Fixture f;
    f.regs[2] = 0x1000;
    f.regs[3] = 0xABCD;
    const auto e = f.run("sh x3, 8(x2)");
    auto& s = GET_ALT(e, instr_effect::store_mem<uint16_t>);
    CHECK_EQ(s.addr, static_cast<uint64_t>(0x1008));
    CHECK_EQ(s.value, static_cast<uint16_t>(0xABCD));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(sw_word_store)
{
    Fixture f;
    f.regs[2] = 0x1000;
    f.regs[3] = 0xDEADBEEF;
    const auto e = f.run("sw x3, 8(x2)");
    auto& s = GET_ALT(e, instr_effect::store_mem<uint32_t>);
    CHECK_EQ(s.addr, static_cast<uint64_t>(0x1008));
    CHECK_EQ(s.value, static_cast<uint32_t>(0xDEADBEEF));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(sd_doubleword_store)
{
    Fixture f;
    f.regs[2] = 0x1000;
    f.regs[3] = 0xDEADBEEFCAFEBABEULL;
    const auto e = f.run("sd x3, 8(x2)");
    auto& s = GET_ALT(e, instr_effect::store_mem<uint64_t>);
    CHECK_EQ(s.addr, static_cast<uint64_t>(0x1008));
    CHECK_EQ(s.value, static_cast<uint64_t>(0xDEADBEEFCAFEBABEULL));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

// ===========================================================================
// RV64I word ops  ->  update_rd (32-bit op, sign-extended to 64)
//   ADDIW, SLLIW, SRLIW, SRAIW, ADDW, SUBW, SLLW, SRLW, SRAW
// ===========================================================================

TEST(addiw_sign_extends_32bit_result)
{
    Fixture f;
    f.regs[2] = 0x8000'0000ULL; // bit 31 set
    const auto e = f.run("addiw x1, x2, 0");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(0xFFFF'FFFF'8000'0000ULL));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(addiw_wraps_at_32_bits)
{
    Fixture f;
    f.regs[2] = 0xFFFF'FFFFULL;
    const auto e = f.run("addiw x1, x2, 1"); // wraps to 0
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(0));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(slliw_sign_extends_result)
{
    Fixture f;
    f.regs[2] = 0x0800'0000ULL; // << 4 -> 0x8000'0000 (bit 31 set)
    const auto e = f.run("slliw x1, x2, 4");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(0xFFFF'FFFF'8000'0000ULL));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(srliw_operates_on_low_32_bits)
{
    Fixture f;
    f.regs[2] = 0xFFFF'FFFF'FFFF'FFFFULL; // upper bits must be ignored
    const auto e = f.run("srliw x1, x2, 4"); // 0xFFFFFFFF >>l 4 = 0x0FFFFFFF
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(0x0FFF'FFFFULL));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(sraiw_arithmetic_shift_of_low_32_bits)
{
    Fixture f;
    f.regs[2] = 0x8000'0000ULL; // negative 32-bit value
    const auto e = f.run("sraiw x1, x2, 4"); // -> 0xF8000000 sign-extended
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(0xFFFF'FFFF'F800'0000ULL));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(addw_sign_extends_32bit_sum)
{
    Fixture f;
    f.regs[2] = 0x7FFF'FFFFULL;
    f.regs[3] = 1; // -> 0x8000'0000
    const auto e = f.run("addw x1, x2, x3");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(0xFFFF'FFFF'8000'0000ULL));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(subw_sign_extends_32bit_difference)
{
    Fixture f;
    f.regs[2] = 0;
    f.regs[3] = 1; // 0 - 1 = -1 (32-bit)
    const auto e = f.run("subw x1, x2, x3");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(0xFFFF'FFFF'FFFF'FFFFULL));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(sllw_masks_shift_to_5_bits)
{
    Fixture f;
    f.regs[2] = 1;
    f.regs[3] = 0x3F; // 0x3F & 0x1F = 31
    const auto e = f.run("sllw x1, x2, x3"); // 1 << 31 -> 0x80000000 sign-ext
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(0xFFFF'FFFF'8000'0000ULL));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(srlw_logical_shift_of_low_32_bits)
{
    Fixture f;
    f.regs[2] = 0xFFFF'FFFFULL;
    f.regs[3] = 4;
    const auto e = f.run("srlw x1, x2, x3"); // 0x0FFFFFFF
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(0x0FFF'FFFFULL));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(sraw_arithmetic_shift_of_low_32_bits)
{
    Fixture f;
    f.regs[2] = 0x8000'0000ULL;
    f.regs[3] = 4;
    const auto e = f.run("sraw x1, x2, x3"); // 0xF8000000 sign-ext
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(0xFFFF'FFFF'F800'0000ULL));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

// ===========================================================================
// RV32M  ->  update_rd   (MUL, MULH, MULHSU, MULHU, DIV, DIVU, REM, REMU)
// ===========================================================================

TEST(mul_low_64_bits)
{
    Fixture f;
    f.regs[2] = 6;
    f.regs[3] = 7;
    const auto e = f.run("mul x1, x2, x3");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(42));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(mulh_signed_high_bits)
{
    Fixture f;
    f.regs[2] = 0x1'0000'0000ULL; // 2^32
    f.regs[3] = 0x1'0000'0000ULL; // 2^32  -> product 2^64, high = 1
    const auto e = f.run("mulh x1, x2, x3");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(1));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(mulhu_unsigned_high_bits)
{
    Fixture f;
    f.regs[2] = 0x1'0000'0000ULL;
    f.regs[3] = 0x1'0000'0000ULL;
    const auto e = f.run("mulhu x1, x2, x3");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(1));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(mulhsu_signed_times_unsigned_high_bits)
{
    Fixture f;
    f.regs[2] = static_cast<uint64_t>(-1); // signed -1
    f.regs[3] = 2; // unsigned 2  -> product -2
    const auto e = f.run("mulhsu x1, x2, x3");
    const auto& [rd, value] = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(value, static_cast<uint64_t>(0xFFFF'FFFF'FFFF'FFFFULL)); // high of -2
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(div_signed)
{
    Fixture f;
    f.regs[2] = 20;
    f.regs[3] = 3;
    const auto e = f.run("div x1, x2, x3");
    const auto& [rd, value] = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(value, static_cast<uint64_t>(6));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(div_by_zero_returns_all_ones)
{
    Fixture f;
    f.regs[2] = 20;
    f.regs[3] = 0;
    const auto e = f.run("div x1, x2, x3");
    const auto& [rd, value] = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(value, static_cast<uint64_t>(0xFFFF'FFFF'FFFF'FFFFULL));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(div_signed_overflow_returns_dividend)
{
    Fixture f;
    f.regs[2] = 0x8000'0000'0000'0000ULL; // INT64_MIN
    f.regs[3] = static_cast<uint64_t>(-1); // -1  -> overflow
    const auto e = f.run("div x1, x2, x3");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(0x8000'0000'0000'0000ULL));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(divu_unsigned)
{
    Fixture f;
    f.regs[2] = 20;
    f.regs[3] = 3;
    const auto e = f.run("divu x1, x2, x3");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(6));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(divu_by_zero_returns_all_ones)
{
    Fixture f;
    f.regs[2] = 20;
    f.regs[3] = 0;
    const auto e = f.run("divu x1, x2, x3");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(0xFFFF'FFFF'FFFF'FFFFULL));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(rem_signed)
{
    Fixture f;
    f.regs[2] = 20;
    f.regs[3] = 3;
    const auto e = f.run("rem x1, x2, x3");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(2));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(rem_by_zero_returns_dividend)
{
    Fixture f;
    f.regs[2] = 20;
    f.regs[3] = 0;
    const auto e = f.run("rem x1, x2, x3");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(20));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(rem_signed_overflow_returns_zero)
{
    Fixture f;
    f.regs[2] = 0x8000'0000'0000'0000ULL; // INT64_MIN
    f.regs[3] = static_cast<uint64_t>(-1);
    const auto e = f.run("rem x1, x2, x3");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(0));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(remu_unsigned)
{
    Fixture f;
    f.regs[2] = 20;
    f.regs[3] = 3;
    const auto e = f.run("remu x1, x2, x3");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(2));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(remu_by_zero_returns_dividend)
{
    Fixture f;
    f.regs[2] = 20;
    f.regs[3] = 0;
    const auto e = f.run("remu x1, x2, x3");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(20));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

// ===========================================================================
// RV64M word ops  ->  update_rd (32-bit, sign-extended)
//   MULW, DIVW, DIVUW, REMW, REMUW
// ===========================================================================

TEST(mulw_sign_extends_low_32_product)
{
    Fixture f;
    f.regs[2] = 0xFFFF'FFFFULL; // low32 = -1
    f.regs[3] = 2; // low32 product = 0xFFFFFFFE
    const auto e = f.run("mulw x1, x2, x3");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(0xFFFF'FFFF'FFFF'FFFEULL));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(divw_signed_word)
{
    Fixture f;
    f.regs[2] = 20;
    f.regs[3] = 3;
    const auto e = f.run("divw x1, x2, x3");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(6));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(divuw_unsigned_word)
{
    Fixture f;
    f.regs[2] = 20;
    f.regs[3] = 3;
    const auto e = f.run("divuw x1, x2, x3");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(6));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(remw_sign_follows_dividend)
{
    Fixture f;
    f.regs[2] = 0xFFFF'FFF9ULL; // low32 = -7
    f.regs[3] = 3; // -7 % 3 = -1
    const auto e = f.run("remw x1, x2, x3");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(0xFFFF'FFFF'FFFF'FFFFULL));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(remuw_unsigned_word)
{
    Fixture f;
    f.regs[2] = 20;
    f.regs[3] = 3;
    const auto e = f.run("remuw x1, x2, x3");
    auto& u = GET_ALT(e, instr_effect::update_rd);
    CHECK_EQ(u.value, static_cast<uint64_t>(2));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

// ===========================================================================
// Atomics: LR / SC   (load_reserved, store_conditional)
// ===========================================================================

TEST(lr_w_emits_word_load_reserved)
{
    Fixture f;
    f.regs[2] = 0x2000;
    const auto e = f.run("lr.w x1, (x2)");
    auto& l = GET_ALT(e, instr_effect::load_reserved<uint32_t>);
    CHECK_EQ(static_cast<int>(l.rd), 1);
    CHECK_EQ(l.addr, static_cast<uint64_t>(0x2000));
    CHECK_EQ(l.sign_ext, true);
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(lr_d_emits_doubleword_load_reserved)
{
    Fixture f;
    f.regs[2] = 0x2000;
    const auto e = f.run("lr.d x1, (x2)");
    auto& l = GET_ALT(e, instr_effect::load_reserved<uint64_t>);
    CHECK_EQ(static_cast<int>(l.rd), 1);
    CHECK_EQ(l.addr, static_cast<uint64_t>(0x2000));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(sc_w_emits_word_store_conditional)
{
    Fixture f;
    f.regs[2] = 0x2000; // addr
    f.regs[3] = 0xCAFE; // value
    const auto e = f.run("sc.w x1, x3, (x2)");
    auto& s = GET_ALT(e, instr_effect::store_conditional<uint32_t>);
    CHECK_EQ(static_cast<int>(s.rd), 1);
    CHECK_EQ(s.addr, static_cast<uint64_t>(0x2000));
    CHECK_EQ(s.value, static_cast<uint32_t>(0xCAFE));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(sc_d_emits_doubleword_store_conditional)
{
    Fixture f;
    f.regs[2] = 0x2000;
    f.regs[3] = 0x1122'3344'5566'7788ULL;
    const auto e = f.run("sc.d x1, x3, (x2)");
    auto& s = GET_ALT(e, instr_effect::store_conditional<uint64_t>);
    CHECK_EQ(static_cast<int>(s.rd), 1);
    CHECK_EQ(s.addr, static_cast<uint64_t>(0x2000));
    CHECK_EQ(s.value, static_cast<uint64_t>(0x1122'3344'5566'7788ULL));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

// ===========================================================================
// Atomics: AMO  ->  amo_rmw   (exercises to_amo_type mapping for every type)
//   amo.value == rs2 ; addr == rs1 ; .w => size 4, sign_ext true
// ===========================================================================

// --- 32-bit AMOs: size 4, sign_ext true; addr == rs1, value == rs2 ---------

TEST(amoswap_w_emits_swap_rmw)
{
    Fixture f;
    f.regs[2] = 0x2000;
    f.regs[3] = 0x55;
    const auto e = f.run("amoswap.w x1, x3, (x2)");
    auto& a = GET_ALT(e, instr_effect::amo_rmw<uint32_t>);
    CHECK_EQ(static_cast<int>(a.rd), 1);
    CHECK_EQ(static_cast<int>(a.type), static_cast<int>(amo_type::SWAP));
    CHECK_EQ(a.addr, static_cast<uint64_t>(0x2000));
    CHECK_EQ(a.value, static_cast<uint32_t>(0x55));
    CHECK_EQ(a.sign_ext, true);
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(amoadd_w_emits_add_rmw)
{
    Fixture f;
    f.regs[2] = 0x2000;
    f.regs[3] = 0x55;
    const auto e = f.run("amoadd.w x1, x3, (x2)");
    auto& a = GET_ALT(e, instr_effect::amo_rmw<uint32_t>);
    CHECK_EQ(static_cast<int>(a.rd), 1);
    CHECK_EQ(static_cast<int>(a.type), static_cast<int>(amo_type::ADD));
    CHECK_EQ(a.addr, static_cast<uint64_t>(0x2000));
    CHECK_EQ(a.value, static_cast<uint32_t>(0x55));
    CHECK_EQ(a.sign_ext, true);
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(amoxor_w_emits_xor_rmw)
{
    Fixture f;
    f.regs[2] = 0x2000;
    f.regs[3] = 0x55;
    const auto e = f.run("amoxor.w x1, x3, (x2)");
    auto& a = GET_ALT(e, instr_effect::amo_rmw<uint32_t>);
    CHECK_EQ(static_cast<int>(a.rd), 1);
    CHECK_EQ(static_cast<int>(a.type), static_cast<int>(amo_type::XOR));
    CHECK_EQ(a.addr, static_cast<uint64_t>(0x2000));
    CHECK_EQ(a.value, static_cast<uint32_t>(0x55));
    CHECK_EQ(a.sign_ext, true);
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(amoand_w_emits_and_rmw)
{
    Fixture f;
    f.regs[2] = 0x2000;
    f.regs[3] = 0x55;
    const auto e = f.run("amoand.w x1, x3, (x2)");
    auto& a = GET_ALT(e, instr_effect::amo_rmw<uint32_t>);
    CHECK_EQ(static_cast<int>(a.rd), 1);
    CHECK_EQ(static_cast<int>(a.type), static_cast<int>(amo_type::AND));
    CHECK_EQ(a.addr, static_cast<uint64_t>(0x2000));
    CHECK_EQ(a.value, static_cast<uint32_t>(0x55));
    CHECK_EQ(a.sign_ext, true);
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(amoor_w_emits_or_rmw)
{
    Fixture f;
    f.regs[2] = 0x2000;
    f.regs[3] = 0x55;
    const auto e = f.run("amoor.w x1, x3, (x2)");
    auto& a = GET_ALT(e, instr_effect::amo_rmw<uint32_t>);
    CHECK_EQ(static_cast<int>(a.rd), 1);
    CHECK_EQ(static_cast<int>(a.type), static_cast<int>(amo_type::OR));
    CHECK_EQ(a.addr, static_cast<uint64_t>(0x2000));
    CHECK_EQ(a.value, static_cast<uint32_t>(0x55));
    CHECK_EQ(a.sign_ext, true);
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(amomin_w_emits_min_rmw)
{
    Fixture f;
    f.regs[2] = 0x2000;
    f.regs[3] = 0x55;
    const auto e = f.run("amomin.w x1, x3, (x2)");
    auto& a = GET_ALT(e, instr_effect::amo_rmw<uint32_t>);
    CHECK_EQ(static_cast<int>(a.rd), 1);
    CHECK_EQ(static_cast<int>(a.type), static_cast<int>(amo_type::MIN));
    CHECK_EQ(a.addr, static_cast<uint64_t>(0x2000));
    CHECK_EQ(a.value, static_cast<uint32_t>(0x55));
    CHECK_EQ(a.sign_ext, true);
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(amomax_w_emits_max_rmw)
{
    Fixture f;
    f.regs[2] = 0x2000;
    f.regs[3] = 0x55;
    const auto e = f.run("amomax.w x1, x3, (x2)");
    auto& a = GET_ALT(e, instr_effect::amo_rmw<uint32_t>);
    CHECK_EQ(static_cast<int>(a.rd), 1);
    CHECK_EQ(static_cast<int>(a.type), static_cast<int>(amo_type::MAX));
    CHECK_EQ(a.addr, static_cast<uint64_t>(0x2000));
    CHECK_EQ(a.value, static_cast<uint32_t>(0x55));
    CHECK_EQ(a.sign_ext, true);
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(amominu_w_emits_minu_rmw)
{
    Fixture f;
    f.regs[2] = 0x2000;
    f.regs[3] = 0x55;
    const auto e = f.run("amominu.w x1, x3, (x2)");
    auto& a = GET_ALT(e, instr_effect::amo_rmw<uint32_t>);
    CHECK_EQ(static_cast<int>(a.rd), 1);
    CHECK_EQ(static_cast<int>(a.type), static_cast<int>(amo_type::MINU));
    CHECK_EQ(a.addr, static_cast<uint64_t>(0x2000));
    CHECK_EQ(a.value, static_cast<uint32_t>(0x55));
    CHECK_EQ(a.sign_ext, true);
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(amomaxu_w_emits_maxu_rmw)
{
    Fixture f;
    f.regs[2] = 0x2000;
    f.regs[3] = 0x55;
    const auto e = f.run("amomaxu.w x1, x3, (x2)");
    auto& a = GET_ALT(e, instr_effect::amo_rmw<uint32_t>);
    CHECK_EQ(static_cast<int>(a.rd), 1);
    CHECK_EQ(static_cast<int>(a.type), static_cast<int>(amo_type::MAXU));
    CHECK_EQ(a.addr, static_cast<uint64_t>(0x2000));
    CHECK_EQ(a.value, static_cast<uint32_t>(0x55));
    CHECK_EQ(a.sign_ext, true);
    CHECK_EQ(e.new_pc, f.pc + 4);
}

// --- 64-bit AMOs: sign_ext moot for 8-byte, not asserted -----------------

TEST(amoswap_d_emits_swap_rmw)
{
    Fixture f;
    f.regs[2] = 0x2000;
    f.regs[3] = 0x55;
    const auto e = f.run("amoswap.d x1, x3, (x2)");
    auto& a = GET_ALT(e, instr_effect::amo_rmw<uint64_t>);
    CHECK_EQ(static_cast<int>(a.rd), 1);
    CHECK_EQ(static_cast<int>(a.type), static_cast<int>(amo_type::SWAP));
    CHECK_EQ(a.addr, static_cast<uint64_t>(0x2000));
    CHECK_EQ(a.value, static_cast<uint64_t>(0x55));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(amoadd_d_emits_add_rmw)
{
    Fixture f;
    f.regs[2] = 0x2000;
    f.regs[3] = 0x55;
    const auto e = f.run("amoadd.d x1, x3, (x2)");
    auto& a = GET_ALT(e, instr_effect::amo_rmw<uint64_t>);
    CHECK_EQ(static_cast<int>(a.rd), 1);
    CHECK_EQ(static_cast<int>(a.type), static_cast<int>(amo_type::ADD));
    CHECK_EQ(a.addr, static_cast<uint64_t>(0x2000));
    CHECK_EQ(a.value, static_cast<uint64_t>(0x55));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(amoxor_d_emits_xor_rmw)
{
    Fixture f;
    f.regs[2] = 0x2000;
    f.regs[3] = 0x55;
    const auto e = f.run("amoxor.d x1, x3, (x2)");
    auto& a = GET_ALT(e, instr_effect::amo_rmw<uint64_t>);
    CHECK_EQ(static_cast<int>(a.rd), 1);
    CHECK_EQ(static_cast<int>(a.type), static_cast<int>(amo_type::XOR));
    CHECK_EQ(a.addr, static_cast<uint64_t>(0x2000));
    CHECK_EQ(a.value, static_cast<uint64_t>(0x55));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(amoand_d_emits_and_rmw)
{
    Fixture f;
    f.regs[2] = 0x2000;
    f.regs[3] = 0x55;
    const auto e = f.run("amoand.d x1, x3, (x2)");
    auto& a = GET_ALT(e, instr_effect::amo_rmw<uint64_t>);
    CHECK_EQ(static_cast<int>(a.rd), 1);
    CHECK_EQ(static_cast<int>(a.type), static_cast<int>(amo_type::AND));
    CHECK_EQ(a.addr, static_cast<uint64_t>(0x2000));
    CHECK_EQ(a.value, static_cast<uint64_t>(0x55));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(amoor_d_emits_or_rmw)
{
    Fixture f;
    f.regs[2] = 0x2000;
    f.regs[3] = 0x55;
    const auto e = f.run("amoor.d x1, x3, (x2)");
    auto& a = GET_ALT(e, instr_effect::amo_rmw<uint64_t>);
    CHECK_EQ(static_cast<int>(a.rd), 1);
    CHECK_EQ(static_cast<int>(a.type), static_cast<int>(amo_type::OR));
    CHECK_EQ(a.addr, static_cast<uint64_t>(0x2000));
    CHECK_EQ(a.value, static_cast<uint64_t>(0x55));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(amomin_d_emits_min_rmw)
{
    Fixture f;
    f.regs[2] = 0x2000;
    f.regs[3] = 0x55;
    const auto e = f.run("amomin.d x1, x3, (x2)");
    auto& a = GET_ALT(e, instr_effect::amo_rmw<uint64_t>);
    CHECK_EQ(static_cast<int>(a.rd), 1);
    CHECK_EQ(static_cast<int>(a.type), static_cast<int>(amo_type::MIN));
    CHECK_EQ(a.addr, static_cast<uint64_t>(0x2000));
    CHECK_EQ(a.value, static_cast<uint64_t>(0x55));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(amomax_d_emits_max_rmw)
{
    Fixture f;
    f.regs[2] = 0x2000;
    f.regs[3] = 0x55;
    const auto e = f.run("amomax.d x1, x3, (x2)");
    auto& a = GET_ALT(e, instr_effect::amo_rmw<uint64_t>);
    CHECK_EQ(static_cast<int>(a.rd), 1);
    CHECK_EQ(static_cast<int>(a.type), static_cast<int>(amo_type::MAX));
    CHECK_EQ(a.addr, static_cast<uint64_t>(0x2000));
    CHECK_EQ(a.value, static_cast<uint64_t>(0x55));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(amominu_d_emits_minu_rmw)
{
    Fixture f;
    f.regs[2] = 0x2000;
    f.regs[3] = 0x55;
    const auto e = f.run("amominu.d x1, x3, (x2)");
    auto& a = GET_ALT(e, instr_effect::amo_rmw<uint64_t>);
    CHECK_EQ(static_cast<int>(a.rd), 1);
    CHECK_EQ(static_cast<int>(a.type), static_cast<int>(amo_type::MINU));
    CHECK_EQ(a.addr, static_cast<uint64_t>(0x2000));
    CHECK_EQ(a.value, static_cast<uint64_t>(0x55));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(amomaxu_d_emits_maxu_rmw)
{
    Fixture f;
    f.regs[2] = 0x2000;
    f.regs[3] = 0x55;
    const auto e = f.run("amomaxu.d x1, x3, (x2)");
    auto& a = GET_ALT(e, instr_effect::amo_rmw<uint64_t>);
    CHECK_EQ(static_cast<int>(a.rd), 1);
    CHECK_EQ(static_cast<int>(a.type), static_cast<int>(amo_type::MAXU));
    CHECK_EQ(a.addr, static_cast<uint64_t>(0x2000));
    CHECK_EQ(a.value, static_cast<uint64_t>(0x55));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

// ===========================================================================
// CSR  ->  csr_rmw   (CSRRW, CSRRS, CSRRC, CSRRWI, CSRRSI, CSRRCI)
//   register form: value == rs1 ; immediate form: value == zext(uimm5)
//   addr == CSR number (mscratch == 0x340)
// ===========================================================================

TEST(csrrw_register_form)
{
    Fixture f;
    f.regs[2] = 0x1234;
    const auto e = f.run("csrrw x1, mscratch, x2");
    auto& c = GET_ALT(e, instr_effect::csr_rmw);
    CHECK_EQ(static_cast<int>(c.rd), 1);
    CHECK_EQ(static_cast<int>(c.type), static_cast<int>(csr_op_type::RW));
    CHECK_EQ(c.addr, static_cast<uint16_t>(csr_register::MSCRATCH));
    CHECK_EQ(c.value, static_cast<uint64_t>(0x1234));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(csrrs_register_form)
{
    Fixture f;
    f.regs[2] = 0x00FF;
    const auto e = f.run("csrrs x1, mscratch, x2");
    auto& c = GET_ALT(e, instr_effect::csr_rmw);
    CHECK_EQ(static_cast<int>(c.type), static_cast<int>(csr_op_type::RS));
    CHECK_EQ(c.addr, static_cast<uint16_t>(csr_register::MSCRATCH));
    CHECK_EQ(c.value, static_cast<uint64_t>(0x00FF));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(csrrc_register_form)
{
    Fixture f;
    f.regs[2] = 0x00FF;
    const auto e = f.run("csrrc x1, mscratch, x2");
    auto& c = GET_ALT(e, instr_effect::csr_rmw);
    CHECK_EQ(static_cast<int>(c.type), static_cast<int>(csr_op_type::RC));
    CHECK_EQ(c.addr, static_cast<uint16_t>(csr_register::MSCRATCH));
    CHECK_EQ(c.value, static_cast<uint64_t>(0x00FF));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(csrrwi_immediate_form)
{
    constexpr Fixture f;
    const auto e = f.run("csrrwi x1, mscratch, 5");
    auto& c = GET_ALT(e, instr_effect::csr_rmw);
    CHECK_EQ(static_cast<int>(c.type), static_cast<int>(csr_op_type::RW));
    CHECK_EQ(c.addr, static_cast<uint16_t>(csr_register::MSCRATCH));
    CHECK_EQ(c.value, static_cast<uint64_t>(5)); // zero-extended 5-bit imm
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(csrrsi_immediate_form)
{
    constexpr Fixture f;
    const auto e = f.run("csrrsi x1, mscratch, 5");
    auto& c = GET_ALT(e, instr_effect::csr_rmw);
    CHECK_EQ(static_cast<int>(c.type), static_cast<int>(csr_op_type::RS));
    CHECK_EQ(c.value, static_cast<uint64_t>(5));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(csrrci_immediate_form_max_imm)
{
    constexpr Fixture f;
    const auto e = f.run("csrrci x1, mscratch, 31"); // 0x1F, max 5-bit
    auto& c = GET_ALT(e, instr_effect::csr_rmw);
    CHECK_EQ(static_cast<int>(c.type), static_cast<int>(csr_op_type::RC));
    CHECK_EQ(c.value, static_cast<uint64_t>(0x1F));
    CHECK_EQ(e.new_pc, f.pc + 4);
}

// ===========================================================================
// MISC_MEM  ->  no_effect   (FENCE, FENCE_TSO, PAUSE, FENCE_I)
// If your assembler lacks Zihintpause/Ztso/Zifencei mnemonics, the raw encodings
// are noted as .word fallbacks.
// ===========================================================================

TEST(fence_is_no_effect)
{
    constexpr Fixture f;
    const auto e = f.run("fence"); // fallback: .word 0x0ff0000f
    (void)GET_ALT(e, instr_effect::no_effect);
    CHECK_EQ(e.new_pc, f.pc + 4);
}

TEST(fence_tso_is_no_effect)
{
    constexpr Fixture f;
    const auto e = f.run("fence.tso"); // fallback: .word 0x8330000f
    (void)GET_ALT(e, instr_effect::no_effect);
    CHECK_EQ(e.new_pc, f.pc + 4);
}

// Figure out that pause extension
// TEST(pause_is_no_effect)
// {
//     constexpr Fixture f;
//     const auto e = f.run("pause"); // fallback: .word 0x0100000f
//     (void)GET_ALT(e, instr_effect::no_effect);
//     CHECK_EQ(e.new_pc, f.pc + 4);
// }

TEST(fence_i_is_no_effect)
{
    constexpr Fixture f;
    const auto e = f.run("fence.i"); // fallback: .word 0x0000100f
    (void)GET_ALT(e, instr_effect::no_effect);
    CHECK_EQ(e.new_pc, f.pc + 4);
}

// ===========================================================================
// SYSTEM traps  ->  raise_trap   (ECALL, EBREAK) + illegal instruction
// new_pc not asserted (the CPU loop sets the trap vector).
// ===========================================================================

TEST(ecall_raises_environment_call)
{
    constexpr Fixture f; // assumes default privilege == M
    const auto e = f.run("ecall");
    auto& t = GET_ALT(e, instr_effect::raise_trap);
    CHECK_EQ(t.cause.is_interrupt, false);
    CHECK_EQ(t.cause.code, static_cast<uint64_t>(exception_type::ECALL_M));
    // If your Fixture defaults to U or S, change the expected code to ECALL_U/ECALL_S.
}

TEST(ebreak_raises_breakpoint)
{
    constexpr Fixture f;
    const auto e = f.run("ebreak");
    auto& t = GET_ALT(e, instr_effect::raise_trap);
    CHECK_EQ(t.cause.is_interrupt, false);
    CHECK_EQ(t.cause.code, static_cast<uint64_t>(exception_type::BREAKPOINT));
    // tval (mtval) semantics for breakpoints vary by implementation; not asserted.
}

TEST(illegal_instruction_raises_trap)
{
    constexpr Fixture f;
    const auto e = f.run(".word 0x00000000"); // all-zero word is illegal
    auto& t = GET_ALT(e, instr_effect::raise_trap);
    CHECK_EQ(t.cause.is_interrupt, false);
    CHECK_EQ(t.cause.code, static_cast<uint64_t>(exception_type::ILLEGAL_INSTRUCTION));
    CHECK_EQ(t.tval, static_cast<uint64_t>(0)); // tval == raw instruction bits
    // If run() can't assemble ".word", use your raw-word entry point instead,
    // e.g. f.run_raw(0x00000000).
}

// ===========================================================================
// Trap return  ->  trap_return   (SRET, MRET)
// return_priv is assumed to identify the return type (MRET->M, SRET->S);
// new_pc not asserted (target comes from xepc, which execute() can't read).
// ===========================================================================

TEST(mret_emits_machine_trap_return)
{
    constexpr Fixture f;
    const auto e = f.run("mret");
    auto& r = GET_ALT(e, instr_effect::trap_return);
    CHECK_EQ(static_cast<int>(r.return_priv), static_cast<int>(privilege_level::M));
}

TEST(sret_emits_supervisor_trap_return)
{
    constexpr Fixture f;
    const auto e = f.run("sret");
    auto& r = GET_ALT(e, instr_effect::trap_return);
    CHECK_EQ(static_cast<int>(r.return_priv), static_cast<int>(privilege_level::S));
}

// ===========================================================================
// WFI  ->  handle_wfi
// ===========================================================================

TEST(wfi_emits_handle_wfi)
{
    constexpr Fixture f;
    const auto e = f.run("wfi");
    (void)GET_ALT(e, instr_effect::handle_wfi);
    CHECK_EQ(e.new_pc, f.pc + 4);
}

// ===========================================================================
// main: arg parsing + run loop (each test wrapped in try/catch)
// ===========================================================================
namespace {
enum class match_policy
{
    exact,
    contains,
    prefix
};

// string -> enum, via the same reflection loop enum_name uses in reverse.
template <class E>
    requires std::is_enum_v<E>
std::optional<E> enum_from_name(const std::string_view s)
{
    template for (constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^E)))
    {
        if (s == std::meta::identifier_of(e))
            return [:e:];
    }
    return std::nullopt;
}

void print_usage(const char* argv0)
{
    std::print("usage: {} [--list] [--filter=POLICY] [NAME...]\n"
               "  NAME...          run only tests matching a given NAME (all if none given)\n"
               "  --filter=POLICY  how NAMEs match: exact (default) | contains\n"
               "  --list, -l       print selected test names, one per line\n"
               "exit code: 0 = all selected passed, 1 = at least one failed, 2 = bad args.\n",
               argv0);
}
} // namespace

int main(const int argc, char** argv)
{
    bool list = false;
    auto policy = match_policy::exact;
    std::vector<std::string_view> patterns;

    int i = 1;
    for (; i < argc; ++i) { // flags phase
        const std::string_view a = argv[i];
        if (!a.starts_with('-'))
            break; // first non-flag: switch phases
        if (a == "--list" || a == "-l") {
            list = true;
        }
        else if (a.starts_with("--filter=")) {
            const auto p = enum_from_name<match_policy>(a.substr(9));
            if (!p) {
                std::println(stderr, "unknown filter policy: {}", a.substr(9));
                print_usage(argv[0]);
                return 2;
            }
            policy = *p;
        }
        else if (a == "--help" || a == "-h") {
            print_usage(argv[0]);
            return 0;
        }
        else {
            std::println(stderr, "unknown argument: {}", a);
            print_usage(argv[0]);
            return 2;
        }
    }
    for (; i < argc; ++i)
        patterns.emplace_back(argv[i]); // trailing names

    const auto selected = [&](const std::string_view name)
    {
        if (patterns.empty()) {
            return true;
        }

        std::function<bool(std::string_view)> predicate;

        switch (policy) {
            using enum match_policy;

        case exact:
            predicate = [&](const std::string_view pattern) { return name == pattern; };
            break;
        case contains:
            predicate = [&](const std::string_view pattern) { return name.find(pattern) != std::string_view::npos; };
            break;
        case prefix:
            predicate = [&](const std::string_view pattern) { return name.rfind(pattern, 0) == 0; };
            break;
        default:
            std::unreachable();
        };
        return std::ranges::any_of(patterns, predicate);
    };

    if (list) {
        for (const auto& name : registry() | std::views::keys) {
            if (selected(name)) {
                std::cout << name << "\n";
            }
        }
        return 0;
    }

    int passed = 0, failed = 0;
    for (const auto& [name, fn] : registry()) {
        if (!selected(name))
            continue;
        try {
            fn();
            std::cout << "PASS: " << name << std::endl;
            ++passed;
        }
        catch (const TestFailure& f) {
            std::cout << "FAIL: " << name << "\n      " << indent(f.msg) << std::endl;
            ++failed;
        }
        catch (const std::exception& e) {
            std::cout << "FAIL: " << name << "\n      uncaught std::exception: " << e.what() << std::endl;
            ++failed;
        }
        catch (...) {
            std::cout << "FAIL: " << name << "\n      uncaught unknown exception" << std::endl;
            ++failed;
        }
    }

    if (passed + failed > 0) {
        std::cout << "\n" << "------------------------------------------------------------" << "\n";
        std::print("SUMMARY: {}/{} passed", passed, passed + failed);
        if (failed > 0) {
            std::print(", {} failed", failed);
        }
        std::cout << "\n" << "------------------------------------------------------------" << "\n";
    }

    if (failed > 0) {
        return 1;
    }
    return 0;
}
