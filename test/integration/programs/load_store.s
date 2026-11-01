# Every load width/signedness, every store width, and memory contents
    .data
vals:
    .dword 0x8899AABBCCDDEEFF   # bytes in memory: FF EE DD CC BB AA 99 88

    .text
    .globl _start
_start:
    la a0, vals
    lb  s0, 0(a0)           # 0xFF -> -1
    lbu s1, 0(a0)           # 0xFF
    lh  s2, 0(a0)           # 0xEEFF -> 0xFFFFFFFFFFFFEEFF
    lhu s3, 0(a0)           # 0xEEFF
    lw  s4, 4(a0)           # 0x8899AABB -> 0xFFFFFFFF8899AABB
    lwu s5, 4(a0)           # 0x8899AABB
    ld  s6, 0(a0)           # 0x8899AABBCCDDEEFF
    lb  s7, 7(a0)           # 0x88 -> 0xFFFFFFFFFFFFFF88

    li a1, 0x80010000       # scratch area in RAM, well past the program
    li t0, 0x1122334455667788
    sd t0, 0(a1)
    sw t0, 8(a1)            # 0x55667788
    sh t0, 12(a1)           # 0x7788
    sb t0, 14(a1)           # 0x88
    ld  s8, 0(a1)           # 0x1122334455667788
    lwu s9, 8(a1)           # 0x55667788
    lhu s10, 12(a1)         # 0x7788
    lbu s11, 14(a1)         # 0x88
halt:
    j halt
