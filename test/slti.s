    .text
    .globl _start
_start:
    addi x1, x0, -5
    slti x2, x1, 0
    slti x3, x1, -10
    sltiu x4, x1, 1
    wfi
