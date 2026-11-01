# CSR read/write/set/clear, register and immediate forms, fixed-value CSRs
    .text
    .globl _start
_start:
    csrr s0, misa           # RV64 + A, I, M
    csrr s1, mhartid        # 0
    li t0, 0x1234
    csrw mscratch, t0
    csrr s2, mscratch       # 0x1234
    li t1, 0xF
    csrrs s3, mscratch, t1  # old 0x1234; new 0x123F
    csrr s4, mscratch       # 0x123F
    li t2, 0x30
    csrrc s5, mscratch, t2  # old 0x123F; new 0x120F
    csrr s6, mscratch       # 0x120F
    csrrwi s7, mscratch, 7  # old 0x120F; new 7
    csrr s8, mscratch       # 7
    csrr s9, mstatus        # reset value: MPP = M (0x1800)
halt:
    j halt
