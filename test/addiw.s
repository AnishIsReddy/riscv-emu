    .text
    .globl _start
_start:
    li x1, 0x000000007fffffff
    addiw x2, x1, 1
    addiw x3, x0, -1
    wfi
