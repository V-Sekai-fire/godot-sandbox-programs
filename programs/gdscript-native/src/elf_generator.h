#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace gdscript {

// Simple ELF file generator for RISC-V 64 Linux
// Creates minimal ELF executable with code and symbols
class ELFGenerator {
public:
    /// \brief Add a code section with machine code
    /// \param code Pointer to machine code bytes
    /// \param size Size of code in bytes
    /// \param name Section name (default: ".text")
    void add_code_section(const uint8_t* code, size_t size, const std::string& name = ".text");
    
    /// \brief Add a function symbol (for sandbox to discover)
    /// \param name Function name
    /// \param address Virtual address of function
    /// \param size Size of function code in bytes
    void add_symbol(const std::string& name, uint64_t address, size_t size);
    
    /// \brief Generate ELF file and return as byte array
    /// \return Vector containing ELF file bytes, or empty vector on failure
    /// \note This method does not modify internal state (could be const but returns new data)
    std::vector<uint8_t> generate() const;
    
    /// \brief Clear all sections and symbols
    void clear();

private:
    struct CodeSection {
        std::vector<uint8_t> data;
        std::string name;
        uint64_t address;
    };
    
    struct Symbol {
        std::string name;
        uint64_t address;
        size_t size;
    };
    
    std::vector<CodeSection> _code_sections;
    std::vector<Symbol> _symbols;
    
    // ELF64 constants
    static constexpr uint8_t ELF_MAGIC[] = {0x7f, 'E', 'L', 'F'};
    static constexpr uint8_t ELF_CLASS_64 = 2;
    static constexpr uint8_t ELF_DATA_LITTLE = 1;
    static constexpr uint8_t ELF_VERSION = 1;
    static constexpr uint16_t ELF_TYPE_EXEC = 2;
    static constexpr uint16_t ELF_MACHINE_RISCV = 243;
    // ELF_ENTRY_POINT moved to constants.h
    
    // Write ELF header
    void _write_elf_header(std::vector<uint8_t>& elf, size_t& offset, size_t shdr_offset, size_t num_sections, uint64_t entry_point) const;
    
    // Write program headers
    void _write_program_headers(std::vector<uint8_t>& elf, size_t& offset, size_t p_filesz, size_t p_memsz, size_t p_offset, uint64_t p_vaddr) const;
    
    // Write section headers
    void _write_section_headers(std::vector<uint8_t>& elf, size_t shdr_offset, size_t code_offset, size_t code_size, size_t shstrtab_offset, size_t shstrtab_size, uint64_t text_vaddr) const;
    
    // Write symbol table
    void _write_symbol_table(std::vector<uint8_t>& elf, size_t& offset) const;
    
    // Write string table
    void _write_string_table(std::vector<uint8_t>& elf, size_t offset) const;
};

} // namespace gdscript

