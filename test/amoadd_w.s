# amoadd_w.s
    la      x5, scratch
    li      x6, 1
    amoadd.w x7, x6, (x5)       # old → x7; mem = old + 1 (32-bit wrap)
    lw      x28, 0(x5)
    ebreak
.align 2
.data
scratch: .word 0xFFFFFFFF