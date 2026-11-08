# A counted loop plus every branch type, taken and not taken
    .text
    .globl _start
_start:
    li a0, 0
    li a1, 1
    li a2, 11
loop:
    add  a0, a0, a1
    addi a1, a1, 1
    bne  a1, a2, loop

    li s0, 0
    li t0, -1
    li t1, 1

    blt  t0, t1, 1f
    j 2f
1:  ori  s0, s0, 1
2:  bltu t0, t1, 3f
    ori  s0, s0, 2
3:  bge  t1, t0, 4f
    j 5f
4:  ori  s0, s0, 4
5:  bgeu t0, t1, 6f
    j 7f
6:  ori  s0, s0, 8
7:  beq  t0, t0, 8f
    j 9f
8:  ori  s0, s0, 16
9:  beq  t0, t1, 10f
    ori  s0, s0, 32
10:
halt:
    li   x28, 0x100000
    li   x29, 0x5555
    sw   x29, 0(x28)
