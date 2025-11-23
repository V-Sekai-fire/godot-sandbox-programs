// Test to see if pages are being created
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
        options.verbose_loader = true;
        
        std::string_view binary_view{reinterpret_cast<const char*>(binary.data()), binary.size()};
        
        std::cout << "Creating machine...\n";
        Machine<RISCV64> machine{binary_view, options};
        
        std::cout << "Machine created. Entry point: 0x" << std::hex << machine.cpu.pc() << std::dec << "\n";
        std::cout << "Execute segment empty: " << machine.cpu.current_execute_segment().empty() << "\n";
        
        // Try to check if page exists
        address_type<RISCV64> entry = machine.cpu.pc();
        address_type<RISCV64> pageno = entry / riscv::Page::size();
        std::cout << "Entry page number: " << pageno << "\n";
        
        // Try to get the page (this might throw)
        try {
            const auto& page = machine.memory.get_exec_pageno(pageno);
            std::cout << "Page found! exec=" << page.attr.exec << " read=" << page.attr.read << " write=" << page.attr.write << "\n";
        } catch (const std::exception& e) {
            std::cout << "Page not found: " << e.what() << "\n";
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
