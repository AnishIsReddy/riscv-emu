    .text
    .globl _start
_start:
    addi x1, x0, 1
    addi x2, x0, 31
    sllw x3, x1, x2
    srlw x4, x3, x2
    sraw x5, x3, x2
    wfi
