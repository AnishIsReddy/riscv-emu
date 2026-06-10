    .text
    .globl _start
_start:
    la   x6, scratch
    addi x1, x0, 0x7f
    sb   x1, 0(x6)
    lb   x2, 0(x6)
    addi x3, x0, -1
    sb   x3, 1(x6)
    lb   x4, 1(x6)
    lbu  x5, 1(x6)
    wfi

    .data
scratch:
    .space 4