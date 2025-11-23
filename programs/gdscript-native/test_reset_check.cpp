// Test to see what happens during reset
#include <libriscv/machine.hpp>
#include <fstream>
#include <vector>
#include <iostream>

using namespace riscv;

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    
    std::string elf_path = argv[1];
    std::ifstream file(elf_path, std::ios::binary);
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> binary(size);
    file.read(reinterpret_cast<char*>(binary.data()), size);
    file.close();
    
    try {
        MachineOptions<RISCV64> options;
        options.memory_max = 64 << 20;
        options.allow_write_exec_segment = true;
        options.protect_segments = false;
        
        std::string_view binary_view{reinterpret_cast<const char*>(binary.data()), binary.size()};
        
        std::cout << "Creating machine (this calls cpu.reset())...\n";
        Machine<RISCV64> machine{binary_view, options};
        
        std::cout << "Machine created successfully\n";
        std::cout << "PC after reset: 0x" << std::hex << machine.cpu.pc() << std::dec << "\n";
        
        // Now try to simulate
        machine.setup_linux({"test_program"});
        machine.setup_linux_syscalls();
        machine.set_max_instructions(10); // Just a few instructions
        
        std::cout << "Starting simulation...\n";
        machine.simulate();
        
        std::cout << "Simulation completed!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
