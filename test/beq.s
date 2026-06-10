    .text
    .globl _start
_start:
    addi x1, x0, 5
    addi x2, x0, 5
    addi x3, x0, 6
    beq x1, x2, taken
    addi x4, x0, 1
taken:
    addi x5, x0, 2
    beq x1, x3, skip
    addi x6, x0, 3
skip:
    wfi
