# amoadd_d.s
    la      x5, scratch
    li      x6, 1
    amoadd.d x7, x6, (x5)
    ld      x28, 0(x5)
    ebreak
.align 3
.data
scratch: .dword 0x00000000FFFFFFFF