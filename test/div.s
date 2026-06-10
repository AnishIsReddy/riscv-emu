    .text
    .globl _start
_start:
    addi x1, x0, 20
    addi x2, x0, 6
    div x3, x1, x2
    rem x4, x1, x2
    divu x5, x1, x2
    remu x6, x1, x2
    wfi
