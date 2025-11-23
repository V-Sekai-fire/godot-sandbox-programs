#include <api.hpp>
#include <cstring>
#include <sys/mman.h>
#include <memory>
#include <sstream>

// Direct AST to RISC-V emitter using biscuit (following BEAM JIT pattern)
#include "ast_to_riscv_biscuit.h"
// #include "test_data_loader.h"  // TODO: Add test_data_loader files
#include "parser/gdscript_parser.h"
#include "function_registry.h"
#include "code_memory_manager.h"
#include "elf_generator.h"

using AsmCallback = Variant(*)();

// Global function registry to track compiled functions
static gdscript::FunctionRegistry g_functionRegistry;

// Global memory manager to track executable memory allocations
static gdscript::CodeMemoryManager g_memoryManager;

// Compile GDScript to RISC-V ELF file (following Godot sandbox calling convention)
static Variant compile_gdscript(String gdscript_code) {
	// Parse GDScript code to AST
	gdscript::GDScriptParser parser;
	if (!parser.is_valid()) {
		print("Error: Parser initialization failed\n");
		return Nil;
	}
	
	std::string source = gdscript_code.utf8().get_data();
	std::unique_ptr<gdscript::ProgramNode> ast = parser.parse(source);
	
	if (!ast) {
		print("Error: Failed to parse GDScript code\n");
		print("Error message: ", parser.getErrorMessage().c_str(), "\n");
		return Nil;
	}
	
	// Convert AST directly to RISC-V machine code using biscuit (like BEAM JIT with AsmJit)
	gdscript::ASTToRISCVEmitter emitter;
	std::pair<const uint8_t*, size_t> emit_result = emitter.emit(ast.get());
	const uint8_t* machineCode = emit_result.first;
	size_t codeSize = emit_result.second;
	
	if (machineCode == nullptr || codeSize == 0) {
		print("Error: Failed to emit RISC-V machine code\n");
		return Nil;
	}
	
	print("Successfully parsed GDScript code with ", ast->functions.size(), " function(s)\n");
	if (!ast->functions.empty()) {
		print("First function: ", ast->functions[0]->name.c_str(), "\n");
	}
	print("Generated ", codeSize, " bytes of RISC-V machine code\n");
	
	// Generate ELF file from machine code
	gdscript::ELFGenerator elf_gen;
	elf_gen.add_code_section(machineCode, codeSize, ".text");
	
	// Add function symbols for sandbox to discover
	// Functions are at entry point (0x10000) for now
	// TODO: Calculate actual function addresses when we support multiple functions
	uint64_t func_address = 0x10000;
	for (const std::unique_ptr<gdscript::FunctionNode>& func : ast->functions) {
		std::string funcName = func->name;
		if (funcName.empty()) {
			funcName = "main";
		}
		// For now, all functions start at entry point
		// TODO: Calculate proper offsets when we support multiple functions
		elf_gen.add_symbol(funcName, func_address, codeSize);
	}
	
	// Generate ELF file
	std::vector<uint8_t> elf_data = elf_gen.generate();
	if (elf_data.empty()) {
		print("Error: Failed to generate ELF file\n");
		return Nil;
	}
	
	print("Generated ELF file: ", elf_data.size(), " bytes\n");
	
	// Convert to PackedByteArray for Godot
	PackedByteArray result;
	result.resize(elf_data.size());
	std::memcpy(result.ptrw(), elf_data.data(), elf_data.size());
	
	return result;
}

// Test function: compile and call a simple function
static Variant test_compile() {
	print("Testing GDScript to RISC-V compilation...\n");
	
	// Create a simple function: func hello(): return 42
	String testCode = "func hello():\n    return 42\n";
	
	Variant result = compile_gdscript(testCode);
	
	if (result.get_type() == Variant::Type::PACKED_BYTE_ARRAY) {
		PackedByteArray elf_data = result;
		print("Compilation successful! Generated ELF file: ", elf_data.size(), " bytes\n");
		return Variant(elf_data.size()); // Return size as success indicator
	} else {
		print("Compilation failed\n");
		return Nil;
	}
}

// Load and test with dataset entries
// TODO: Re-enable when test_data_loader is added
static Variant test_dataset(int count) {
	print("Error: test_data_loader not available\n");
		return Nil;
}

// Get a random entry from the dataset
// TODO: Re-enable when test_data_loader is added
static Variant get_random_test() {
	print("Error: test_data_loader not available\n");
		return Nil;
}

int main() {
	print("GDScript to RISC-V Compiler (direct AST to RISC-V, no MLIR)\n");
	
	// Add public API
	ADD_API_FUNCTION(compile_gdscript, "PackedByteArray", "String gdscript_code", 
	                 "Compile GDScript to RISC-V and return a callable function");
	ADD_API_FUNCTION(test_compile, "Variant", "", 
	                 "Test the compilation system with a simple function");
	ADD_API_FUNCTION(test_dataset, "Variant", "int count", 
	                 "Test compilation with entries from the GDScript dataset (count: 1-100, default: 10)");
	ADD_API_FUNCTION(get_random_test, "String", "", 
	                 "Get a random GDScript code sample from the dataset");
	
	halt();
}
