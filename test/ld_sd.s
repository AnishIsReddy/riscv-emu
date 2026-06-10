    .text
    .globl _start
_start:
    li x1, 0x123456789abcdef0
    sd x1, 0(x0)
    ld x2, 0(x0)
    wfi
