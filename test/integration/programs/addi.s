# ADDI with positive, negative and maximum immediates
    .text
    .globl _start
_start:
    addi x1, x0, 100
    addi x2, x1, -50
    addi x3, x0, -1
    addi x4, x0, 0x7FF
halt:
    li   x28, 0x100000
    li   x29, 0x5555
    sw   x29, 0(x28)
