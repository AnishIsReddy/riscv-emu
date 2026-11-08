# Every load width/signedness, every store width, and memory contents
    .data
vals:
    .dword 0x8899AABBCCDDEEFF

    .text
    .globl _start
_start:
    la a0, vals
    lb  s0, 0(a0)
    lbu s1, 0(a0)
    lh  s2, 0(a0)
    lhu s3, 0(a0)
    lw  s4, 4(a0)
    lwu s5, 4(a0)
    ld  s6, 0(a0)
    lb  s7, 7(a0)

    li a1, 0x80010000
    li t0, 0x1122334455667788
    sd t0, 0(a1)
    sw t0, 8(a1)
    sh t0, 12(a1)
    sb t0, 14(a1)
    ld  s8, 0(a1)
    lwu s9, 8(a1)
    lhu s10, 12(a1)
    lbu s11, 14(a1)
halt:
    li   x28, 0x100000
    li   x29, 0x5555
    sw   x29, 0(x28)
