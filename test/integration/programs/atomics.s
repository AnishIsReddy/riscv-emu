# AMOs (.W results are sign-extended), LR/SC success and failure
    .text
    .globl _start
_start:
    li a0, 0x80010000
    li t0, 0x7FFFFFFF
    sw t0, 0(a0)

    li t1, 1
    amoadd.w  s0, t1, (a0)
    li t2, 5
    amoswap.w s1, t2, (a0)
    li t3, -3
    amomax.w  s2, t3, (a0)
    amominu.w s3, t3, (a0)
    amomin.w  s4, t3, (a0)
    lw s5, 0(a0)
    amomaxu.w s6, t2, (a0)

    li a1, 0x80010008
    li t4, 42
    sd t4, 0(a1)
    lr.d s7, (a1)
    li t5, 99
    sc.d s8, t5, (a1)
    ld s9, 0(a1)
    sc.d s10, t4, (a1)

    lr.w t6, (a0)
    sw x0, 0(a0)
    sc.w s11, t4, (a0)
halt:
    li   x28, 0x100000
    li   x29, 0x5555
    sw   x29, 0(x28)
