# ECALL from M-mode: mcause, mepc, MRET back past the ecall, mstatus after MRET
    .text
    .globl _start
_start:
    la t0, handler
    csrw mtvec, t0
    li s0, 0
ecall_site:
    ecall
    la t1, ecall_site
    sub t2, s2, t1
    seqz s3, t2
    csrr s4, mstatus
    j halt

handler:
    addi s0, s0, 1
    csrr s1, mcause
    csrr s2, mepc
    addi t3, s2, 4
    csrw mepc, t3
    mret

halt:
    li   x28, 0x100000
    li   x29, 0x5555
    sw   x29, 0(x28)
