    .text
    .globl _start
_start:
    addi x1, x0, 10
    div x2, x1, x0
    divu x3, x1, x0
    rem x4, x1, x0
    remu x5, x1, x0
    li x6, 0x8000000000000000
    addi x7, x0, -1
    div x8, x6, x7
    rem x9, x6, x7
    wfi
