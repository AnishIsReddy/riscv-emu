    .text
    .globl _start
_start:
    la x6, scratch
    lui x1, 0x80000
    sw x1, 0(x0)
    lw x2, 0(x0)
    lwu x3, 0(x0)
    wfi

    .data
scratch:
    .space 4
