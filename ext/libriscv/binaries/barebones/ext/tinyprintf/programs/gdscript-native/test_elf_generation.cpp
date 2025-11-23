// Test program to generate ELF file and save it for inspection
#include "parser/gdscript_parser.h"
#include "ast_to_riscv_biscuit.h"
#include "elf_generator.h"
#include <iostream>
#include <fstream>
#include <string>

using namespace gdscript;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <output.elf> [gdscript_code]\n";
        std::cerr << "Example: " << argv[0] << " test.elf \"func test():\\n    return 42\\n\"\n";
        return 1;
    }
    
    std::string output_file = argv[1];
    std::string gdscript_code;
    
    if (argc >= 3) {
        std::string arg = argv[2];
        // Try to read as file first
        std::ifstream file(arg);
        if (file.good()) {
            gdscript_code.assign((std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>());
            file.close();
        } else {
            // Use as code string, replace literal \n with actual newlines
            gdscript_code = arg;
            std::string::size_type pos = 0;
            while ((pos = gdscript_code.find("\\n", pos)) != std::string::npos) {
                gdscript_code.replace(pos, 2, "\n");
                pos += 1;
            }
        }
    } else {
        // Default test code
        gdscript_code = "func test():\n    return 42\n";
    }
    
    std::cout << "Parsing GDScript code:\n" << gdscript_code << "\n\n";
    
    // Parse
    GDScriptParser parser;
    if (!parser.is_valid()) {
        std::cerr << "Error: Parser initialization failed\n";
        return 1;
    }
    
    auto ast = parser.parse(gdscript_code);
    if (!ast) {
        std::cerr << "Error: Failed to parse GDScript code\n";
        std::cerr << "Error: " << parser.getErrorMessage() << "\n";
        return 1;
    }
    
    std::cout << "Successfully parsed " << ast->functions.size() << " function(s)\n";
    
    // Emit RISC-V code
    ASTToRISCVEmitter emitter;
    auto [machineCode, codeSize] = emitter.emit(ast.get());
    
    if (machineCode == nullptr || codeSize == 0) {
        std::cerr << "Error: Failed to emit RISC-V machine code\n";
        return 1;
    }
    
    std::cout << "Generated " << codeSize << " bytes of RISC-V machine code\n";
    
    // Generate ELF file
    ELFGenerator elf_gen;
    elf_gen.add_code_section(machineCode, codeSize, ".text");
    
    // Add function symbols
    uint64_t func_address = 0x10000; // Entry point
    for (const auto& func : ast->functions) {
        std::string funcName = func->name;
        if (funcName.empty()) {
            funcName = "main";
        }
        elf_gen.add_symbol(funcName, func_address, codeSize);
        std::cout << "Added symbol: " << funcName << " at 0x" << std::hex << func_address << std::dec << "\n";
    }
    
    // Generate ELF
    std::vector<uint8_t> elf_data = elf_gen.generate();
    if (elf_data.empty()) {
        std::cerr << "Error: Failed to generate ELF file\n";
        return 1;
    }
    
    std::cout << "Generated ELF file: " << elf_data.size() << " bytes\n";
    
    // Save to file
    std::ofstream out(output_file, std::ios::binary);
    if (!out) {
        std::cerr << "Error: Failed to open output file: " << output_file << "\n";
        return 1;
    }
    
    out.write(reinterpret_cast<const char*>(elf_data.data()), elf_data.size());
    out.close();
    
    std::cout << "Saved ELF file to: " << output_file << "\n";
    std::cout << "\nNow you can inspect it with:\n";
    std::cout << "  riscv64-unknown-elf-readelf -a " << output_file << "\n";
    std::cout << "  riscv64-unknown-elf-objdump -d " << output_file << "\n";
    std::cout << "  file " << output_file << "\n";
    
    return 0;
}

