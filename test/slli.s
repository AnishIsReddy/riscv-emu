    .text
    .globl _start
_start:
    addi x1, x0, 1
    slli x2, x1, 10
    slli x3, x1, 63
    wfi
