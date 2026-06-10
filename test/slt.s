    .text
    .globl _start
_start:
    addi x1, x0, -1
    addi x2, x0, 1
    slt x3, x1, x2
    slt x4, x2, x1
    sltu x5, x1, x2
    sltu x6, x2, x1
    wfi
