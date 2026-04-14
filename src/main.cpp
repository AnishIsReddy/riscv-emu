#include <fstream>

#include "machine.h"

std::vector<uint8_t> load_binary_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    const auto size = file.tellg();
    file.seekg(0);
    std::vector<uint8_t> binary(size);
    file.read(reinterpret_cast<char*>(binary.data()), size);
    return binary;
}

int main()
{
    riscv_emu::machine machine;

    auto binary = load_binary_file("test.bin");
    machine.load(binary.data(), binary.size());
    machine.run();
    return 0;
}
