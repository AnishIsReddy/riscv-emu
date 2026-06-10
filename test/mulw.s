    .text
    .globl _start
_start:
    li x1, 0x7fffffff
    addi x2, x0, 2
    mulw x3, x1, x2
    addi x4, x0, -3
    addi x5, x0, 7
    mulw x6, x4, x5
    wfi
