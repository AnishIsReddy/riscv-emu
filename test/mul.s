    .text
    .globl _start
_start:
    addi x1, x0, 7
    addi x2, x0, 6
    mul x3, x1, x2
    addi x4, x0, -3
    mul x5, x1, x4
    wfi
