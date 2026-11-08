# Illegal instruction, load/store access faults, and an access to a CSR that
# doesn't exist. The handler logs each mcause into s1 (one byte per trap) and
# sums mtval into s3.
    .text
    .globl _start
_start:
    la t0, handler
    csrw mtvec, t0
    li s0, 0
    li s1, 0
    li s3, 0

    .word 0x00000000
    li a0, 0x10
    li a1, 0x55
    lw a1, 0(a0)
    mv s4, a1
    sw a1, 0(a0)
    csrw 0x7C0, x0
    j halt

handler:
    addi s0, s0, 1
    csrr t1, mcause
    slli s1, s1, 8
    or   s1, s1, t1
    csrr t2, mtval
    add  s3, s3, t2
    csrr t3, mepc
    addi t3, t3, 4
    csrw mepc, t3
    mret

halt:
    li   x28, 0x100000
    li   x29, 0x5555
    sw   x29, 0(x28)
