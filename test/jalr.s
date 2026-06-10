    .text
    .globl _start
_start:
    auipc x1, 0
    jalr x2, x1, 16
    addi x3, x0, 99
    addi x4, x0, 42
    wfi
