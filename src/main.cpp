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

int main(const int argc, char* argv[])
{
    riscv_emu::machine machine;

    std::string filename = "test.bin";
    if (argc == 2) {
        filename = std::string(argv[1]);
    }

    const auto binary = load_binary_file(filename);
    machine.load(binary.data(), binary.size());
    machine.run();
    machine.dump(std::cout);
    return 0;
}
