// Debug simulate to see what's happening
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
        Machine<RISCV64> machine{binary_view, options};
        
        address_type<RISCV64> pc = machine.cpu.pc();
        std::cout << "PC: 0x" << std::hex << pc << std::dec << "\n";
        
        // Check if executable
        std::cout << "is_executable(PC): " << machine.cpu.is_executable(pc) << "\n";
        
        // Try to read instruction directly
        try {
            auto instr = machine.cpu.read_next_instruction();
            std::cout << "First instruction: 0x" << std::hex << instr.whole << std::dec << "\n";
        } catch (const std::exception& e) {
            std::cerr << "Error reading instruction: " << e.what() << "\n";
        }
        
        // Now try simulate
        machine.setup_linux({"test_program"});
        machine.setup_linux_syscalls();
        machine.set_max_instructions(1);
        
        std::cout << "Starting simulate()...\n";
        machine.simulate();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
