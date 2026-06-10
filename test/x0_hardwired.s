    .text
    .globl _start
_start:
    addi x0, x0, 100
    lui x0, 0x12345
    add x1, x0, x0
    wfi
