#!/bin/bash
riscv64-unknown-elf-gcc -nostdlib -march=rv64im -mabi=lp64 -o test.elf "$1"
riscv64-unknown-elf-objcopy -O binary test.elf test.bin
