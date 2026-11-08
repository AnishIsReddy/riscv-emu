# CSR read/write/set/clear, register and immediate forms, fixed-value CSRs
    .text
    .globl _start
_start:
    csrr s0, misa
    csrr s1, mhartid
    li t0, 0x1234
    csrw mscratch, t0
    csrr s2, mscratch
    li t1, 0xF
    csrrs s3, mscratch, t1
    csrr s4, mscratch
    li t2, 0x30
    csrrc s5, mscratch, t2
    csrr s6, mscratch
    csrrwi s7, mscratch, 7
    csrr s8, mscratch
    csrr s9, mstatus
halt:
    li   x28, 0x100000
    li   x29, 0x5555
    sw   x29, 0(x28)
