    .text
    .globl _start
_start:
    addi x1, x0, 5
    addi x2, x0, 5
    addi x3, x0, 3
    bge x1, x2, taken1
    addi x4, x0, 1
taken1:
    bge x1, x3, taken2
    addi x5, x0, 1
taken2:
    addi x6, x0, 2
    wfi
