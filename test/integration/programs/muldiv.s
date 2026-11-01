# M extension, including the spec-defined divide-by-zero and overflow results
    .text
    .globl _start
_start:
    li a0, -7
    li a1, 2
    mul    s0, a0, a1       # -14
    li a2, 0x100000000      # 2^32
    mulh   s1, a2, a2       # 2^64 -> high half 1
    mulhu  s2, a0, a1       # (2^64 - 7) * 2 -> high half 1
    mulhsu s3, a0, a1       # -7 * 2 = -14 -> high half -1
    div    s4, a0, a1       # -3 (rounds toward zero)
    rem    s5, a0, a1       # -1 (sign of dividend)
    divu   s6, a0, a1       # 0x7FFFFFFFFFFFFFFC
    remu   s7, a0, a1       # 1
    div    s8, a0, x0       # divide by zero -> -1
    rem    s9, a0, x0       # remainder by zero -> dividend (-7)
    li a3, 0x8000000000000000
    li a4, -1
    div    s10, a3, a4      # overflow -> INT64_MIN
    rem    s11, a3, a4      # overflow -> 0

    li t0, 0x7FFFFFFF
    mulw   t1, t0, a1       # low 32 = 0xFFFFFFFE -> -2
    divw   t2, a0, a1       # -3
    li t3, 0x80000000
    divw   t5, t3, a4       # 32-bit overflow -> 0xFFFFFFFF80000000
    remuw  t6, a0, x0       # by zero -> dividend low 32 (0xFFFFFFF9), sign-extended
halt:
    j halt
