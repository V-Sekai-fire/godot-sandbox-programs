// Test program to load and execute ELF file using libriscv
#include <libriscv/machine.hpp>
#include <libriscv/rsp_server.hpp>
#include <libriscv/debug.hpp>
#include <fstream>
#include <vector>
#include <iostream>
#include <cstring>
#include <cstdlib>

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
        // Configure machine options
        MachineOptions<RISCV64> options;
        options.memory_max = 64 << 20; // 64 MiB
        options.allow_write_exec_segment = true;
        options.protect_segments = false;
        
        // Enable verbose logging if DEBUG environment variable is set
        const char* debug_env = std::getenv("DEBUG");
        options.verbose_loader = (debug_env != nullptr && std::string(debug_env) == "1");
        
        // Create RISC-V machine with binary (using string_view, like Godot sandbox)
        std::string_view binary_view{reinterpret_cast<const char*>(binary.data()), binary.size()};
        Machine<RISCV64> machine{binary_view, options};
        
        std::cout << "Machine created successfully\n";
        std::cout << "Entry point: 0x" << std::hex << machine.cpu.pc() << std::dec << "\n";
        std::cout << "Start address: 0x" << std::hex << machine.memory.start_address() << std::dec << "\n";
        
        // Setup Linux environment (like Godot sandbox does)
        machine.setup_linux({"test_program"});
        machine.setup_linux_syscalls();
        
        std::cout << "Linux environment setup complete\n";
        
        machine.set_max_instructions(1'000'000);
        
        // Use debugger if DEBUG=1, otherwise run normally
        const char* debug_env = std::getenv("DEBUG");
        bool use_debugger = (debug_env != nullptr && std::string(debug_env) == "1");
        
        if (use_debugger) {
            std::cout << "\n=== Using Built-in Debugger ===\n";
            riscv::DebugMachine debug{machine};
            debug.verbose_instructions = true;
            debug.print_and_pause();
            
            std::cout << "Starting execution with debug output...\n";
            try {
                debug.simulate();
            } catch (riscv::MachineException& me) {
                std::cerr << ">>> Machine exception " << me.type() << ": " << me.what() 
                          << " (data: 0x" << std::hex << me.data() << std::dec << ")\n";
                debug.print_and_pause();
            } catch (std::exception& e) {
                std::cerr << ">>> General exception: " << e.what() << "\n";
                debug.print_and_pause();
            }
        } else {
            std::cout << "Starting execution...\n";
            machine.simulate();
        }
        
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
