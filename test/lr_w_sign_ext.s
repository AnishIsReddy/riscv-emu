    .text
    .globl _start
_start:
    la      x5, scratch
    lr.w    x6, (x5)            # sign-extension under test
    sc.w    x7, x6, (x5)        # pairs the LR; stores same low word back
    lwu     x28, 0(x5)          # zero-extending read-back
    wfi

    .data
    .align 2
scratch:
    .word 0x80000000