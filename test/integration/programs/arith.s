# Register/immediate ALU ops, LUI/AUIPC, and x0 staying zero
    .text
    .globl _start
_start:
    auipc t6, 0
    auipc t5, 1

    li a0, 1000
    li a1, -300
    add  s0, a0, a1
    sub  s1, a0, a1
    li a2, 0xF0F0
    li a3, 0x0FF0
    and  s2, a2, a3
    or   s3, a2, a3
    xor  s4, a2, a3
    slt  s5, a1, a0
    sltu s6, a1, a0
    slti s7, a1, -299
    sltiu s8, a0, -1
    lui  s9, 0x12345
    lui  s10, 0xFFFFF

    addi x0, x0, 5
    add  s11, x0, x0
halt:
    li   x28, 0x100000
    li   x29, 0x5555
    sw   x29, 0(x28)
