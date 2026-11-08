# M extension, including the spec-defined divide-by-zero and overflow results
    .text
    .globl _start
_start:
    li a0, -7
    li a1, 2
    mul    s0, a0, a1
    li a2, 0x100000000
    mulh   s1, a2, a2
    mulhu  s2, a0, a1
    mulhsu s3, a0, a1
    div    s4, a0, a1
    rem    s5, a0, a1
    divu   s6, a0, a1
    remu   s7, a0, a1
    div    s8, a0, x0
    rem    s9, a0, x0
    li a3, 0x8000000000000000
    li a4, -1
    div    s10, a3, a4
    rem    s11, a3, a4

    li t0, 0x7FFFFFFF
    mulw   t1, t0, a1
    divw   t2, a0, a1
    li t3, 0x80000000
    divw   t5, t3, a4
    remuw  t6, a0, x0
halt:
    li   x28, 0x100000
    li   x29, 0x5555
    sw   x29, 0(x28)
