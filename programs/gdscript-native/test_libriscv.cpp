// Test program to load and execute ELF file using libriscv
// Following the same pattern as Godot sandbox
#include <libriscv/machine.hpp>
#include <fstream>
#include <vector>
#include <iostream>
#include <cstring>

using namespace riscv;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <elf_file>\n";
        std::cerr << "Example: " << argv[0] << " test_output/simple.elf\n";
        return 1;
    }
    
    std::string elf_path = argv[1];
    
    // Read ELF file
    std::ifstream file(elf_path, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Cannot open file: " << elf_path << "\n";
        return 1;
    }
    
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> binary(size);
    file.read(reinterpret_cast<char*>(binary.data()), size);
    file.close();
    
    std::cout << "=== Testing ELF with libriscv ===\n";
    std::cout << "Loaded ELF file: " << elf_path << " (" << size << " bytes)\n";
    
    try {
        // Create machine options (like Godot sandbox does)
        MachineOptions<RISCV64> options;
        options.memory_max = 64 << 20; // 64 MiB
        
        // Create RISC-V machine with binary (using string_view, like Godot sandbox)
        std::string_view binary_view{reinterpret_cast<const char*>(binary.data()), binary.size()};
        Machine<RISCV64> machine{binary_view, options};
        
        std::cout << "Machine created successfully\n";
        std::cout << "Entry point: 0x" << std::hex << machine.cpu.pc() << std::dec << "\n";
        
        // Setup Linux environment (like Godot sandbox does)
        machine.setup_linux({"test_program"});
        machine.setup_linux_syscalls();
        
        std::cout << "Linux environment setup complete\n";
        
        // Set instruction limit
        machine.set_max_instructions(1'000'000);
        
        // Run the program through initialization (like Godot sandbox)
        std::cout << "Starting execution...\n";
        machine.simulate();
        
        // Get exit code (from a0 register, which is register 10)
        // In RISC-V Linux ABI, exit code is returned in a0
        int64_t exit_code = machine.cpu.reg(10); // a0 register
        std::cout << "\n=== Results ===\n";
        std::cout << "Program exited with code: " << exit_code << "\n";
        std::cout << "Instructions executed: " << machine.instruction_counter() << "\n";
        
        if (exit_code == 42) {
            std::cout << "✅ SUCCESS: Program returned 42 as expected!\n";
        } else if (exit_code == 34) {
            std::cout << "⚠️  Program returned 34 (expected 42) - encoding issue\n";
        } else {
            std::cout << "Program returned: " << exit_code << "\n";
        }
        
        return static_cast<int>(exit_code);
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
