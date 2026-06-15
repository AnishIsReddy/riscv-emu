# amoswap_w.s
    la      x5, scratch
    li      x6, 0xAAAAAAAA
    amoswap.w x7, x6, (x5)
    lw      x28, 0(x5)
    ebreak
.align 2
.data
scratch: .word 0x12345678