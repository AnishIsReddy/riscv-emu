# amoand_w.s
    la      x5, scratch
    li      x6, 0xFFFFFFFFF0F0F0F0    # dirty upper 32; only low 32 may apply
    amoand.w x7, x6, (x5)
    lw      x28, 0(x5)
    ebreak
.align 2
.data
scratch: .word 0xFFFF00FF