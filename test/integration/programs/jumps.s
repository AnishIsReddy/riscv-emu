# JAL/JALR calls, the link register, and JALR clearing bit 0 of the target
    .text
    .globl _start
_start:
    li a0, 5
    jal ra, square_plus_one
ret_site:
    mv s0, a0
    la t2, ret_site
    sub t3, ra, t2
    seqz s3, t3

    la t0, square_plus_one
    li a0, 3
    jalr ra, 0(t0)
    mv s1, a0

    la t1, square_plus_one
    addi t1, t1, 1
    li a0, 4
    jalr ra, 0(t1)
    mv s2, a0
    j halt

square_plus_one:
    mul  a0, a0, a0
    addi a0, a0, 1
    ret

halt:
    li   x28, 0x100000
    li   x29, 0x5555
    sw   x29, 0(x28)
