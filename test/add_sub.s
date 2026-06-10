    .text
    .globl _start
_start:
    addi x1, x0, 10
    addi x2, x0, 20
    add x3, x1, x2
    sub x4, x1, x2
    sub x5, x2, x1
    wfi
