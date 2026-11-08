# RV64 *W ops: operate on the low 32 bits, sign-extend the result
    .text
    .globl _start
_start:
    li a0, 0x7FFFFFFF
    li a1, 1
    addw  s0, a0, a1
    addiw s1, a0, 1
    li a2, 0x100000005
    addiw s2, a2, 0
    subw  s3, x0, a1
    li a3, 0x80000000
    sraiw s4, a3, 4
    srliw s5, a3, 4
    li a4, 36
    sllw  s6, a1, a4
    srlw  s7, a3, a4
    sraw  s8, a3, a4
    slliw s9, a1, 31
halt:
    li   x28, 0x100000
    li   x29, 0x5555
    sw   x29, 0(x28)
