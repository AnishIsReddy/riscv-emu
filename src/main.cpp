#include <iostream>

#include "machine.h"

int main()
{
    riscv_emu::machine machine;
    machine.run();
    return 0;
}
