    .text
    .globl _start
_start:
    lui x1, 0x80000
    srliw x2, x1, 1
    srliw x3, x1, 31
    wfi
