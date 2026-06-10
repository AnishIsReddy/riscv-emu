    .text
    .globl _start
_start:
    addi x1, x0, 0xff
    xori x2, x1, 0x0f
    ori x3, x1, 0x100
    andi x4, x1, 0x0f
    wfi
