# 64-bit shifts; register shift amounts use only the low 6 bits
    .text
    .globl _start
_start:
    li a0, -16              # 0xFFFFFFFFFFFFFFF0
    li a1, 1
    slli s0, a1, 63         # 0x8000000000000000
    srli s1, a0, 60         # 0xF
    srai s2, a0, 2          # -4
    li a2, 68               # 68 & 63 == 4
    sll  s3, a1, a2         # 16
    srl  s4, a0, a2         # 0x0FFFFFFFFFFFFFFF
    sra  s5, a0, a2         # -1
halt:
    j halt
