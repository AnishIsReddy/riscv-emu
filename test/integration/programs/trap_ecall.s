# ECALL from M-mode: mcause, mepc, MRET back past the ecall, mstatus after MRET
    .text
    .globl _start
_start:
    la t0, handler
    csrw mtvec, t0
    li s0, 0                # number of traps taken
ecall_site:
    ecall
    la t1, ecall_site
    sub t2, s2, t1
    seqz s3, t2             # 1 if mepc pointed at the ecall
    csrr s4, mstatus        # after MRET: MPIE = 1, MPP = M -> 0x1880
    j halt

handler:
    addi s0, s0, 1
    csrr s1, mcause         # 11 = environment call from M-mode
    csrr s2, mepc
    addi t3, s2, 4
    csrw mepc, t3
    mret

halt:
    j halt
