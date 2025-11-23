// Minimal test to generate ELF and save it
// This bypasses the full build system and can be compiled manually
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>

// Simple ELF header structure for RISC-V 64
struct ELF64Header {
    unsigned char e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

struct ELF64ProgramHeader {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
};

// Simple RISC-V code: return 42
// li a0, 42  (load immediate 42 into a0)
// ret        (return)
std::vector<uint8_t> generate_simple_code() {
    std::vector<uint8_t> code;
    // li a0, 42
    // Encoding: LUI + ADDI
    // LUI a0, 0 (upper 20 bits of 42 are 0)
    code.push_back(0x37); // LUI opcode
    code.push_back(0x05); // rd = a0 (5)
    code.push_back(0x00);
    code.push_back(0x00);
    
    // ADDI a0, a0, 42
    code.push_back(0x13); // ADDI opcode
    code.push_back(0x05); // rd = a0, rs1 = a0
    code.push_back(0x25); // imm[11:0] = 42 (0x2A)
    code.push_back(0x02);
    
    // ret (JALR x0, x1, 0)
    code.push_back(0x67); // JALR opcode
    code.push_back(0x80); // rd = x0, rs1 = x1 (ra)
    code.push_back(0x00);
    code.push_back(0x00);
    
    return code;
}

int main(int argc, char* argv[]) {
    std::string output = argc > 1 ? argv[1] : "test_output/simple.elf";
    
    std::vector<uint8_t> code = generate_simple_code();
    std::cout << "Generated " << code.size() << " bytes of RISC-V code\n";
    
    // Create ELF file
    std::vector<uint8_t> elf;
    elf.resize(4096, 0); // 4KB minimum
    
    size_t offset = 0;
    
    // ELF Header
    ELF64Header* hdr = reinterpret_cast<ELF64Header*>(elf.data() + offset);
    hdr->e_ident[0] = 0x7F;
    hdr->e_ident[1] = 'E';
    hdr->e_ident[2] = 'L';
    hdr->e_ident[3] = 'F';
    hdr->e_ident[4] = 2; // 64-bit
    hdr->e_ident[5] = 1; // little endian
    hdr->e_ident[6] = 1; // version
    hdr->e_type = 2; // ET_EXEC
    hdr->e_machine = 243; // RISC-V
    hdr->e_version = 1;
    hdr->e_entry = 0x10000;
    hdr->e_phoff = 64; // After ELF header
    hdr->e_shoff = 0;
    hdr->e_flags = 0;
    hdr->e_ehsize = 64;
    hdr->e_phentsize = 56;
    hdr->e_phnum = 1;
    hdr->e_shentsize = 0;
    hdr->e_shnum = 0;
    hdr->e_shstrndx = 0;
    
    offset = 64;
    
    // Program Header
    ELF64ProgramHeader* phdr = reinterpret_cast<ELF64ProgramHeader*>(elf.data() + offset);
    phdr->p_type = 1; // PT_LOAD
    phdr->p_flags = 0x5; // PF_R | PF_X
    phdr->p_offset = 120; // After header + program header
    phdr->p_vaddr = 0x10000;
    phdr->p_paddr = 0x10000;
    phdr->p_filesz = code.size();
    phdr->p_memsz = code.size();
    phdr->p_align = 0x1000;
    
    offset = 120;
    
    // Copy code
    std::memcpy(elf.data() + offset, code.data(), code.size());
    
    // Write file
    std::ofstream out(output, std::ios::binary);
    if (!out) {
        std::cerr << "Error: Cannot open " << output << "\n";
        return 1;
    }
    
    // Write only what we need
    size_t total_size = offset + code.size();
    out.write(reinterpret_cast<const char*>(elf.data()), total_size);
    out.close();
    
    std::cout << "Wrote ELF file: " << output << " (" << total_size << " bytes)\n";
    std::cout << "\nInspect with:\n";
    std::cout << "  riscv64-unknown-elf-readelf -a " << output << "\n";
    std::cout << "  riscv64-unknown-elf-objdump -d " << output << "\n";
    std::cout << "  file " << output << "\n";
    
    return 0;
}

