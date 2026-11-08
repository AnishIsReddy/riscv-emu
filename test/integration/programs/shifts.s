# 64-bit shifts; register shift amounts use only the low 6 bits
    .text
    .globl _start
_start:
    li a0, -16
    li a1, 1
    slli s0, a1, 63
    srli s1, a0, 60
    srai s2, a0, 2
    li a2, 68
    sll  s3, a1, a2
    srl  s4, a0, a2
    sra  s5, a0, a2
halt:
    li   x28, 0x100000
    li   x29, 0x5555
    sw   x29, 0(x28)
