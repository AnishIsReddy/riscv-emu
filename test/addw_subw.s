    .text
    .globl _start
_start:
    li x1, 0x7FFFFFFF
    addi x2, x0, 1
    addw x3, x1, x2
    addi x4, x0, 10
    addi x5, x0, 20
    subw x6, x4, x5
    wfi
