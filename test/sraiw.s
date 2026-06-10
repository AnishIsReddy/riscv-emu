    .text
    .globl _start
_start:
    lui x1, 0x80000
    sraiw x2, x1, 1
    sraiw x3, x1, 31
    wfi
