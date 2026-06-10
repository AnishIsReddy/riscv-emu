    .text
    .globl _start
_start:
    addi x1, x0, 100
    addi x2, x1, -50
    addi x3, x0, -1
    addi x4, x0, 0x7FF
    wfi
