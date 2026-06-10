    .text
    .globl _start
_start:
    addi x1, x0, 0xff
    addi x2, x0, 0x0f
    xor x3, x1, x2
    or x4, x1, x2
    and x5, x1, x2
    wfi
