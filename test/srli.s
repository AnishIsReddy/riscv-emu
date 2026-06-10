    .text
    .globl _start
_start:
    li x1, 0x8000000000000000
    srli x2, x1, 1
    srli x3, x1, 63
    wfi
