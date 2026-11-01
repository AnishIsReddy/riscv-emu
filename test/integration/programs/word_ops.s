# RV64 *W ops: operate on the low 32 bits, sign-extend the result
    .text
    .globl _start
_start:
    li a0, 0x7FFFFFFF
    li a1, 1
    addw  s0, a0, a1        # 0x80000000 -> 0xFFFFFFFF80000000
    addiw s1, a0, 1         # same
    li a2, 0x100000005      # upper 32 bits are ignored
    addiw s2, a2, 0         # 5
    subw  s3, x0, a1        # -1
    li a3, 0x80000000       # 0x0000000080000000
    sraiw s4, a3, 4         # 0xFFFFFFFFF8000000
    srliw s5, a3, 4         # 0x08000000
    li a4, 36               # 36 & 31 == 4
    sllw  s6, a1, a4        # 16
    srlw  s7, a3, a4        # 0x08000000
    sraw  s8, a3, a4        # 0xFFFFFFFFF8000000
    slliw s9, a1, 31        # 0xFFFFFFFF80000000
halt:
    j halt
