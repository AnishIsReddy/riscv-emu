    .text
    .globl _start
_start:
    addi x1, x0, 20
    addi x2, x0, 6
    divw x3, x1, x2
    remw x4, x1, x2
    divuw x5, x1, x2
    remuw x6, x1, x2
    divw x7, x1, x0
    remw x8, x1, x0
    wfi
