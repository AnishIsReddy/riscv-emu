# Illegal instruction, load/store access faults, and an access to a CSR that
# doesn't exist. The handler logs each mcause into s1 (one byte per trap) and
# sums mtval into s3.
    .text
    .globl _start
_start:
    la t0, handler
    csrw mtvec, t0
    li s0, 0                # trap count
    li s1, 0                # mcause log
    li s3, 0                # mtval sum

    .word 0x00000000        # illegal instruction        -> mcause 2, mtval 0
    li a0, 0x10             # unmapped address
    li a1, 0x55
    lw a1, 0(a0)            # load access fault          -> mcause 5, mtval 0x10
    mv s4, a1               # a faulting load must not write rd: still 0x55
    sw a1, 0(a0)            # store access fault         -> mcause 7, mtval 0x10
    csrw 0x7C0, x0          # nonexistent CSR (write)    -> mcause 2, mtval 0
    j halt

handler:
    addi s0, s0, 1
    csrr t1, mcause
    slli s1, s1, 8
    or   s1, s1, t1
    csrr t2, mtval
    add  s3, s3, t2
    csrr t3, mepc
    addi t3, t3, 4          # skip the faulting instruction
    csrw mepc, t3
    mret

halt:
    j halt
