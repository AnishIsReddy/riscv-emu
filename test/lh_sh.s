.text
    .globl _start
_start:
    la x6, scratch
    li x1, 0x1234
    sh x1, 0(x6)
    lh x2, 0(x6)
    li x3, 0xffff8000
    sh x3, 2(x6)
    lh x4, 2(x6)
    lhu x5, 2(x6)
    wfi

    .data
scratch:
    .space 4