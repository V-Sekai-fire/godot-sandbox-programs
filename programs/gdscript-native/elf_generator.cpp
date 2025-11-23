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
    for (const auto& section : _code_sections) {
        code_size += section.data.size();
    }
    
    // Use actual code size (not aligned)
    // Alignment is handled by p_align in program header for virtual addresses
    // But file size should match actual code size
    
    // Calculate offsets
    // ELF structure: p_offset=0x0, p_vaddr=0x10000, .text sh_offset=0x78, .text sh_addr=0x10078
    size_t elf_header_size = 64; // ELF64 header
    size_t phdr_size = 56; // ELF64 program header
    size_t phdr_offset = elf_header_size;
    size_t code_offset_in_file = elf_header_size + phdr_size; // Code starts after headers (120 bytes = 0x78)
    
    // ELF segment structure: p_offset = 0x0 (segment starts at file offset 0)
    // p_vaddr = 0x10000 (segment base virtual address)
    // Alignment: p_offset % p_align == p_vaddr % p_align (both 0)
    size_t p_align = 0x1000;
    size_t p_offset = 0x0; // Like C ELF - segment starts at file offset 0
    uint64_t p_vaddr = ELF_ENTRY_POINT; // 0x10000 - segment base
    
    // Section headers: null (0) + .text (1) + .shstrtab (2) = 3 sections
    size_t shdr_size = 64; // ELF64 section header size
    size_t num_sections = 3; // null, .text, .shstrtab
    size_t shstrtab_size = 16; // ".text\0.shstrtab\0" = 16 bytes
    
    // Calculate segment size: p_filesz is aligned size that includes headers + code
    size_t segment_size = code_offset_in_file + code_size;
    // Align to page boundary (like C ELF uses 0x1000 alignment)
    segment_size = ((segment_size + p_align - 1) / p_align) * p_align;
    
    // .text section virtual address = p_vaddr + (sh_offset - p_offset)
    uint64_t text_sh_addr = p_vaddr + (code_offset_in_file - p_offset);
    
    // Entry point at start of .text section
    uint64_t entry_point = text_sh_addr;
    
    // p_filesz: aligned segment size (includes headers + code)
    size_t p_filesz = segment_size;
    
    // p_memsz: actual code size in memory
    size_t p_memsz = code_size;
    
    // Calculate layout: segment at p_offset=0, then shstrtab, then section headers
    size_t shstrtab_offset = segment_size;
    size_t shdr_offset = shstrtab_offset + shstrtab_size;
    size_t total_size = shdr_offset + (num_sections * shdr_size);
    
    std::vector<uint8_t> elf(total_size, 0);
    size_t offset = 0;
    
    // Write ELF header (at file offset 0, within the segment)
    _write_elf_header(elf, offset, shdr_offset, num_sections, entry_point);
    
    // Write program header (at file offset 64, within the segment)
    _write_program_headers(elf, phdr_offset, p_filesz, p_memsz, p_offset, p_vaddr);
    
    // Write code sections after headers (at code_offset_in_file = 120)
    offset = code_offset_in_file;
    for (const auto& section : _code_sections) {
        std::memcpy(elf.data() + offset, section.data.data(), section.data.size());
        offset += section.data.size();
    }
    
    // Write string table for section names
    _write_string_table(elf, shstrtab_offset);
    
    // Write section headers
    // .text sh_addr = p_vaddr + (sh_offset - p_offset)
    // This ensures serialize_execute_segment() places code at the correct virtual address
    _write_section_headers(elf, shdr_offset, code_offset_in_file, code_size, shstrtab_offset, shstrtab_size, text_sh_addr);
    
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
    
    // p_offset = offset in file (must be aligned: p_offset % p_align == p_vaddr % p_align)
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = p_offset;
    offset += 8;
    
    // p_vaddr = virtual address (adjusted for alignment to match p_offset)
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = p_vaddr;
    offset += 8;
    
    // p_paddr = physical address (same as virtual)
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = p_vaddr;
    offset += 8;
    
    // p_filesz = size in file (aligned segment size)
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = p_filesz;
    offset += 8;
    
    // p_memsz = size in memory (actual code size)
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = p_memsz;
    offset += 8;
    
    // p_align = alignment (4KB)
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = 0x1000;
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
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = text_vaddr; // sh_addr = p_vaddr (segment base, not entry point)
    offset += 8;
    *reinterpret_cast<uint64_t*>(elf.data() + offset) = code_offset; // sh_offset = code offset
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

