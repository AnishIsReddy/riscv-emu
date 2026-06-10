    .text
    .globl _start
_start:
    addi x1, x0, 1
    addi x2, x0, 10
    sll x3, x1, x2
    li x4, 0x8000000000000000
    addi x5, x0, 1
    srl x6, x4, x5
    sra x7, x4, x5
    wfi
