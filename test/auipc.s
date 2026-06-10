    .text
    .globl _start
_start:
    auipc x1, 0x1
    auipc x2, 0x0
    auipc x3, 0x4
    wfi
