    .text
    .globl _start
_start:
    li x1, 0x8000000000000000
    srai x2, x1, 1
    srai x3, x1, 63
    addi x4, x0, 64
    srai x5, x4, 2
    wfi
