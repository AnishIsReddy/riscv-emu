# AMOs (.W results are sign-extended), LR/SC success and failure
    .text
    .globl _start
_start:
    li a0, 0x80010000
    li t0, 0x7FFFFFFF
    sw t0, 0(a0)

    li t1, 1
    amoadd.w  s0, t1, (a0)  # old 0x7FFFFFFF; mem = 0x80000000
    li t2, 5
    amoswap.w s1, t2, (a0)  # old 0x80000000 -> sign-extended; mem = 5
    li t3, -3
    amomax.w  s2, t3, (a0)  # old 5; signed max(5, -3) = 5
    amominu.w s3, t3, (a0)  # old 5; unsigned min(5, 0xFFFFFFFD) = 5
    amomin.w  s4, t3, (a0)  # old 5; signed min = -3 -> mem = 0xFFFFFFFD
    lw s5, 0(a0)            # 0xFFFFFFFFFFFFFFFD
    amomaxu.w s6, t2, (a0)  # old 0xFFFFFFFD (sign-extended); stays 0xFFFFFFFD

    li a1, 0x80010008
    li t4, 42
    sd t4, 0(a1)
    lr.d s7, (a1)           # 42
    li t5, 99
    sc.d s8, t5, (a1)       # 0 = success
    ld s9, 0(a1)            # 99
    sc.d s10, t4, (a1)      # 1 = failure (reservation already consumed)

    lr.w t6, (a0)           # 0xFFFFFFFD -> sign-extended
    sw x0, 0(a0)            # a store to the reserved word breaks the reservation
    sc.w s11, t4, (a0)      # 1 = failure; memory stays 0
halt:
    j halt
