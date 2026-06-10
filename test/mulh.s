    .text
    .globl _start
_start:
    li x1, 0x7fffffffffffffff
    addi x2, x0, 2
    mulh x3, x1, x2
    mulhu x4, x1, x2
    addi x5, x0, -1
    mulhsu x6, x5, x2
    wfi
