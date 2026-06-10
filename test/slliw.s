    .text
    .globl _start
_start:
    addi x1, x0, 1
    slliw x2, x1, 31
    slliw x3, x1, 10
    wfi
