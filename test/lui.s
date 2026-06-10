    .text
    .globl _start
_start:
    lui x1, 0x12345
    lui x2, 0x80000
    wfi
