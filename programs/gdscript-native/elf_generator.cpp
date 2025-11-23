#include "elf_generator.h"
#include <cstring>
#include <algorithm>

namespace gdscript {

void ELFGenerator::add_code_section(const uint8_t* code, size_t size, const std::string& name) {
    CodeSection section;
    section.data.resize(size);
    std::memcpy(section.data.data(), code, size);
    section.name = name;
    section.address = ELF_ENTRY_POINT; // Start at entry point
    _code_sections.push_back(std::move(section));
}

void ELFGenerator::add_symbol(const std::string& name, uint64_t address, size_t size) {
    Symbol sym;
    sym.name = name;
    sym.address = address;
    sym.size = size;
    _symbols.push_back(std::move(sym));
}

void ELFGenerator::clear() {
    _code_sections.clear();
    _symbols.clear();
}

std::vector<uint8_t> ELFGenerator::generate() {
    if (_code_sections.empty()) {
        return {};
    }
    
    // Calculate sizes
    size_t code_size = 0;
    for (const auto& section : _code_sections) {
        code_size += section.data.size();
    }
    
    // Align code to page boundary (4KB)
    size_t code_size_aligned = (code_size + 0xFFF) & ~0xFFF;
    
    // Calculate offsets
    size_t elf_header_size = 64; // ELF64 header
    size_t phdr_size = 56; // ELF64 program header
    size_t phdr_offset = elf_header_size;
    size_t code_offset = phdr_offset + phdr_size;
    size_t total_size = code_offset + code_size_aligned;
    
    std::vector<uint8_t> elf(total_size, 0);
    size_t offset = 0;
    
    // Write ELF header
    _write_elf_header(elf, offset);
    
    // Write program header
    _write_program_headers(elf, phdr_offset, code_size_aligned);
    
    // Write code sections
    offset = code_offset;
    for (const auto& section : _code_sections) {
        std::memcpy(elf.data() + offset, section.data.data(), section.data.size());
        offset += section.data.size();
    }
    
    return elf;
}

void ELFGenerator::_write_elf_header(std::vector<uint8_t>& elf, size_t& offset) {
    // ELF magic
    std::memcpy(elf.data() + offset, ELF_MAGIC, 4);
    offset += 4;
    
    // e_ident[EI_CLASS] = 64-bit
    elf[offset++] = ELF_CLASS_64;
    
    // e_ident[EI_DATA] = little endian
    elf[offset++] = ELF_DATA_LITTLE;
    
    // e_ident[EI_VERSION] = 1
    elf[offset++] = ELF_VERSION;
    
    // e_ident[EI_OSABI] = System V (0)
    elf[offset++] = 0;
    
    // e_ident[EI_ABIVERSION] = 0
    elf[offset++] = 0;
    
    // e_ident[EI_PAD] = padding (7 bytes)
    offset += 7;
    
    // e_type = ET_EXEC (2)
    *reinterpret_cast<uint16_t*>(elf.data() + offset) = ELF_TYPE_EXEC;
    offset += 2;
    
    // e_machine = EM_RISCV (243)
    *reinterpret_cast<uint16_t*>(elf.data() + offset) = ELF_MACHINE_RISCV;
    offset += 2;
    
    // e_version = 1
    *reinterpret_cast<uint32_t*>(elf.data() + offset) = 1;
    offset += 4;
    
    // e_entry = entry point address
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = ELF_ENTRY_POINT;
    offset += 8;
    
    // e_phoff = program header offset
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = 64; // After ELF header
    offset += 8;
    
    // e_shoff = section header offset (0 for executable)
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = 0;
    offset += 8;
    
    // e_flags = 0
    *reinterpret_cast<uint32_t*>(elf.data() + offset) = 0;
    offset += 4;
    
    // e_ehsize = ELF header size (64)
    *reinterpret_cast<uint16_t*>(elf.data() + offset) = 64;
    offset += 2;
    
    // e_phentsize = program header size (56)
    *reinterpret_cast<uint16_t*>(elf.data() + offset) = 56;
    offset += 2;
    
    // e_phnum = number of program headers (1)
    *reinterpret_cast<uint16_t*>(elf.data() + offset) = 1;
    offset += 2;
    
    // e_shentsize = section header size (0, no sections)
    *reinterpret_cast<uint16_t*>(elf.data() + offset) = 0;
    offset += 2;
    
    // e_shnum = number of section headers (0)
    *reinterpret_cast<uint16_t*>(elf.data() + offset) = 0;
    offset += 2;
    
    // e_shstrndx = section header string table index (0)
    *reinterpret_cast<uint16_t*>(elf.data() + offset) = 0;
    offset += 2;
}

void ELFGenerator::_write_program_headers(std::vector<uint8_t>& elf, size_t& offset, size_t code_size) {
    // PT_LOAD program header
    // p_type = PT_LOAD (1)
    *reinterpret_cast<uint32_t*>(elf.data() + offset) = 1;
    offset += 4;
    
    // p_flags = PF_R | PF_X (readable and executable)
    *reinterpret_cast<uint32_t*>(elf.data() + offset) = 0x5; // PF_R | PF_X
    offset += 4;
    
    // p_offset = offset in file
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = 64 + 56; // After ELF header + program header
    offset += 8;
    
    // p_vaddr = virtual address
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = ELF_ENTRY_POINT;
    offset += 8;
    
    // p_paddr = physical address (same as virtual)
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = ELF_ENTRY_POINT;
    offset += 8;
    
    // p_filesz = size in file
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = code_size;
    offset += 8;
    
    // p_memsz = size in memory
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = code_size;
    offset += 8;
    
    // p_align = alignment (4KB)
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = 0x1000;
    offset += 8;
}

void ELFGenerator::_write_section_headers(std::vector<uint8_t>& elf, size_t& offset) {
    // Not implemented - minimal ELF doesn't need section headers
}

void ELFGenerator::_write_symbol_table(std::vector<uint8_t>& elf, size_t& offset) {
    // Not implemented - minimal ELF doesn't need symbol table
    // Symbols would be added via section headers
}

void ELFGenerator::_write_string_table(std::vector<uint8_t>& elf, size_t& offset) {
    // Not implemented - minimal ELF doesn't need string table
}

} // namespace gdscript

