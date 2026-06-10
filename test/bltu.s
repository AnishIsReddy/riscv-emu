    .text
    .globl _start
_start:
    addi x1, x0, 1
    addi x2, x0, -1
    bltu x1, x2, taken
    addi x3, x0, 1
taken:
    addi x4, x0, 2
    wfi
