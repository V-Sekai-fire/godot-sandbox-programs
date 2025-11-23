#include "elf_generator.h"
#include <cstring>
#include <algorithm>

namespace gdscript {

void ELFGenerator::add_code_section(const uint8_t* code, size_t size, const std::string& name) {
    CodeSection section;
    section.data.resize(size);
    std::memcpy(section.data.data(), code, size);
    section.name = name;
    // Address will be set when generating ELF (based on p_vaddr calculation)
    section.address = 0; // Will be set to p_vaddr during ELF generation
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
    for (const CodeSection& section : _code_sections) {
        code_size += section.data.size();
    }
    
    // Use actual code size (not aligned)
    // Alignment is handled by p_align in program header for virtual addresses
    // But file size should match actual code size
    
    // ELF64 header and program header sizes
    constexpr size_t ELF_HEADER_SIZE = 64;
    constexpr size_t PROGRAM_HEADER_SIZE = 56;
    constexpr size_t PAGE_ALIGN = 0x1000;
    
    size_t phdr_offset = ELF_HEADER_SIZE;
    size_t code_offset_in_file = ELF_HEADER_SIZE + PROGRAM_HEADER_SIZE; // Code starts at offset 0x78
    
    // Program header: segment starts at file offset 0, loaded at virtual address 0x10000
    // Alignment requirement: p_offset % p_align == p_vaddr % p_align (both 0)
    size_t p_offset = 0x0;
    uint64_t p_vaddr = ELF_ENTRY_POINT;
    
    // Section headers: null (0) + .text (1) + .shstrtab (2)
    constexpr size_t SECTION_HEADER_SIZE = 64;
    constexpr size_t NUM_SECTIONS = 3;
    constexpr size_t SHSTRTAB_SIZE = 16; // ".text\0.shstrtab\0"
    
    // Calculate segment size: align to page boundary (includes headers + code)
    size_t segment_size = code_offset_in_file + code_size;
    segment_size = ((segment_size + PAGE_ALIGN - 1) / PAGE_ALIGN) * PAGE_ALIGN;
    
    // .text section virtual address: maintains relationship sh_addr - p_vaddr == sh_offset - p_offset
    // This ensures libriscv's serialize_execute_segment() places code correctly
    uint64_t text_sh_addr = p_vaddr + (code_offset_in_file - p_offset);
    uint64_t entry_point = text_sh_addr;
    
    // Program header sizes
    size_t p_filesz = segment_size;  // File size: aligned segment (includes headers)
    size_t p_memsz = code_size;      // Memory size: actual code only
    
    // File layout: segment (headers + code) -> shstrtab -> section headers
    size_t shstrtab_offset = segment_size;
    size_t shdr_offset = shstrtab_offset + SHSTRTAB_SIZE;
    size_t total_size = shdr_offset + (NUM_SECTIONS * SECTION_HEADER_SIZE);
    
    std::vector<uint8_t> elf(total_size, 0);
    size_t offset = 0;
    
    // Write ELF components in order
    _write_elf_header(elf, offset, shdr_offset, NUM_SECTIONS, entry_point);
    _write_program_headers(elf, phdr_offset, p_filesz, p_memsz, p_offset, p_vaddr);
    
    // Write code sections at code_offset_in_file
    offset = code_offset_in_file;
    for (const CodeSection& section : _code_sections) {
        std::memcpy(elf.data() + offset, section.data.data(), section.data.size());
        offset += section.data.size();
    }
    
    _write_string_table(elf, shstrtab_offset);
    _write_section_headers(elf, shdr_offset, code_offset_in_file, code_size, shstrtab_offset, SHSTRTAB_SIZE, text_sh_addr);
    
    return elf;
}

void ELFGenerator::_write_elf_header(std::vector<uint8_t>& elf, size_t& offset, size_t shdr_offset, size_t num_sections, uint64_t entry_point) {
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
    
    // e_entry = entry point address (code location within segment)
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = entry_point;
    offset += 8;
    
    // e_phoff = program header offset
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = 64; // After ELF header
    offset += 8;
    
    // e_shoff = section header offset
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = shdr_offset;
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
    
    // e_shentsize = section header size (64)
    *reinterpret_cast<uint16_t*>(elf.data() + offset) = 64;
    offset += 2;
    
    // e_shnum = number of section headers
    *reinterpret_cast<uint16_t*>(elf.data() + offset) = num_sections;
    offset += 2;
    
    // e_shstrndx = section header string table index (2 = .shstrtab)
    *reinterpret_cast<uint16_t*>(elf.data() + offset) = 2;
    offset += 2;
}

void ELFGenerator::_write_program_headers(std::vector<uint8_t>& elf, size_t& offset, size_t p_filesz, size_t p_memsz, size_t p_offset, uint64_t p_vaddr) {
    // PT_LOAD program header
    // p_type = PT_LOAD (1)
    *reinterpret_cast<uint32_t*>(elf.data() + offset) = 1;
    offset += 4;
    
    // p_flags = PF_R | PF_X (readable and executable)
    *reinterpret_cast<uint32_t*>(elf.data() + offset) = 0x5; // PF_R | PF_X
    offset += 4;
    
    // Program header fields (PT_LOAD segment)
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = p_offset;  // File offset
    offset += 8;
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = p_vaddr;  // Virtual address
    offset += 8;
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = p_vaddr;  // Physical address (same as virtual)
    offset += 8;
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = p_filesz;  // File size (aligned segment)
    offset += 8;
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = p_memsz;  // Memory size (code only)
    offset += 8;
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = 0x1000;   // Alignment (4KB page)
    offset += 8;
}

void ELFGenerator::_write_section_headers(std::vector<uint8_t>& elf, size_t shdr_offset, size_t code_offset, size_t code_size, size_t shstrtab_offset, size_t shstrtab_size, uint64_t text_vaddr) {
    size_t offset = shdr_offset;
    
    // Section 0: NULL section header
    *reinterpret_cast<uint32_t*>(elf.data() + offset) = 0; // sh_name = 0
    offset += 4;
    *reinterpret_cast<uint32_t*>(elf.data() + offset) = 0; // sh_type = SHT_NULL
    offset += 4;
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = 0; // sh_flags = 0
    offset += 8;
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = 0; // sh_addr = 0
    offset += 8;
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = 0; // sh_offset = 0
    offset += 8;
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = 0; // sh_size = 0
    offset += 8;
    *reinterpret_cast<uint32_t*>(elf.data() + offset) = 0; // sh_link = 0
    offset += 4;
    *reinterpret_cast<uint32_t*>(elf.data() + offset) = 0; // sh_info = 0
    offset += 4;
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = 0; // sh_addralign = 0
    offset += 8;
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = 0; // sh_entsize = 0
    offset += 8;
    
    // Section 1: .text section header
    *reinterpret_cast<uint32_t*>(elf.data() + offset) = 1; // sh_name = offset 1 in .shstrtab (".text")
    offset += 4;
    *reinterpret_cast<uint32_t*>(elf.data() + offset) = 1; // sh_type = SHT_PROGBITS
    offset += 4;
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = 0x6; // sh_flags = SHF_ALLOC | SHF_EXECINSTR
    offset += 8;
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = text_vaddr;  // sh_addr: virtual address
    offset += 8;
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = code_offset; // sh_offset: file offset
    offset += 8;
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = code_size; // sh_size = code size
    offset += 8;
    *reinterpret_cast<uint32_t*>(elf.data() + offset) = 0; // sh_link = 0
    offset += 4;
    *reinterpret_cast<uint32_t*>(elf.data() + offset) = 0; // sh_info = 0
    offset += 4;
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = 4; // sh_addralign = 4
    offset += 8;
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = 0; // sh_entsize = 0
    offset += 8;
    
    // Section 2: .shstrtab section header
    *reinterpret_cast<uint32_t*>(elf.data() + offset) = 7; // sh_name = offset 7 in .shstrtab (".shstrtab")
    offset += 4;
    *reinterpret_cast<uint32_t*>(elf.data() + offset) = 3; // sh_type = SHT_STRTAB
    offset += 4;
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = 0; // sh_flags = 0
    offset += 8;
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = 0; // sh_addr = 0
    offset += 8;
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = shstrtab_offset; // sh_offset = string table offset
    offset += 8;
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = shstrtab_size; // sh_size = string table size
    offset += 8;
    *reinterpret_cast<uint32_t*>(elf.data() + offset) = 0; // sh_link = 0
    offset += 4;
    *reinterpret_cast<uint32_t*>(elf.data() + offset) = 0; // sh_info = 0
    offset += 4;
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = 1; // sh_addralign = 1
    offset += 8;
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = 0; // sh_entsize = 0
    offset += 8;
}

void ELFGenerator::_write_symbol_table(std::vector<uint8_t>& elf, size_t& offset) {
    // Not implemented - minimal ELF doesn't need symbol table
    // Symbols would be added via section headers
}

void ELFGenerator::_write_string_table(std::vector<uint8_t>& elf, size_t offset) {
    // Write .shstrtab: "\0.text\0.shstrtab\0"
    const char* strtab = "\0.text\0.shstrtab\0";
    std::memcpy(elf.data() + offset, strtab, 16);
}

} // namespace gdscript

