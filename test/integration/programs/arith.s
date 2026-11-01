# Register/immediate ALU ops, LUI/AUIPC, and x0 staying zero
    .text
    .globl _start
_start:
    auipc t6, 0             # first instruction: pc == RAM base
    auipc t5, 1             # (RAM base + 4) + 0x1000

    li a0, 1000
    li a1, -300
    add  s0, a0, a1         # 700
    sub  s1, a0, a1         # 1300
    li a2, 0xF0F0
    li a3, 0x0FF0
    and  s2, a2, a3         # 0x00F0
    or   s3, a2, a3         # 0xFFF0
    xor  s4, a2, a3         # 0xFF00
    slt  s5, a1, a0         # signed -300 < 1000 -> 1
    sltu s6, a1, a0         # unsigned huge < 1000 -> 0
    slti s7, a1, -299       # -300 < -299 -> 1
    sltiu s8, a0, -1        # 1000 < 0xFFFF...FFFF -> 1
    lui  s9, 0x12345        # 0x12345000
    lui  s10, 0xFFFFF       # sign-extends: 0xFFFFFFFFFFFFF000

    addi x0, x0, 5          # writes to x0 are discarded
    add  s11, x0, x0        # 0
halt:
    j halt
