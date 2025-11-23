#include <api.hpp>
#include <cstring>
#include <sys/mman.h>
#include <memory>
#include <sstream>

// Direct AST to RISC-V emitter using biscuit (following BEAM JIT pattern)
#include "ast_to_riscv_biscuit.h"
#include "ast_interpreter.h"
// #include "test_data_loader.h"  // TODO: Add test_data_loader files
#include "parser/gdscript_parser.h"
#include "function_registry.h"
#include "code_memory_manager.h"
#include "elf_generator.h"
#include "compiler_mode.h"
#include "constants.h"

using AsmCallback = Variant(*)();

// Global function registry to track compiled functions
static gdscript::FunctionRegistry g_functionRegistry;

// Global memory manager to track executable memory allocations
static gdscript::CodeMemoryManager g_memoryManager;

// Global mode setting (can be changed via API)
// NATIVE_CODE mode is disabled for now - using INTERPRET mode only
// TODO: Re-enable NATIVE_CODE templates once RISC-V execution issues are resolved
static gdscript::CompilerMode g_compiler_mode = gdscript::CompilerMode::INTERPRET;

// Compile GDScript to RISC-V ELF file (following Godot sandbox calling convention)
// Supports two modes: INTERPRET (AST interpreter) and NATIVE_CODE (RISC-V templates)
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
		// Use structured error reporting
		const gdscript::ErrorCollection& errors = parser.get_errors();
		if (errors.has_errors()) {
			std::string error_msg = errors.get_formatted_message();
			print("Error: Failed to parse GDScript code\n");
			print("Error details: ", error_msg.c_str(), "\n");
		} else {
			print("Error: Failed to parse GDScript code\n");
			print("Error message: ", parser.getErrorMessage().c_str(), "\n");
		}
		return Nil;
	}
	
	// Choose execution mode
	if (g_compiler_mode == gdscript::CompilerMode::INTERPRET) {
		// AST Interpreter mode
		gdscript::ASTInterpreter interpreter;
		gdscript::ASTInterpreter::ExecutionResult result = interpreter.execute(ast.get());
		
		if (!result.success) {
			print("Error: Interpreter execution failed: ", result.error_message.c_str(), "\n");
			return Nil;
		}
		
		// Convert return value to Variant
		if (std::holds_alternative<int64_t>(result.return_value)) {
			return Variant(static_cast<int>(std::get<int64_t>(result.return_value)));
		} else if (std::holds_alternative<double>(result.return_value)) {
			return Variant(std::get<double>(result.return_value));
		} else if (std::holds_alternative<bool>(result.return_value)) {
			return Variant(std::get<bool>(result.return_value));
		}
		return Variant(0);
	} else {
		// Native code template mode (like BeamAsm)
		// DISABLED: Native code templates are disabled for now
		// TODO: Re-enable once RISC-V execution issues (PC=0x0) are resolved
		print("Error: NATIVE_CODE mode is disabled. Use INTERPRET mode instead.\n");
		return Nil;
		
		/* DISABLED CODE:
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
		// Functions are at entry point for now
		// TODO: Calculate actual function addresses when we support multiple functions
		uint64_t func_address = gdscript::constants::ELF_ENTRY_POINT;
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
		*/
	}
}

// Set compiler mode (0=INTERPRET, 1=NATIVE_CODE)
// NOTE: NATIVE_CODE mode is disabled - only INTERPRET mode is available
static Variant set_compiler_mode(int mode) {
	if (mode == 0) {
		g_compiler_mode = gdscript::CompilerMode::INTERPRET;
		print("Compiler mode set to: INTERPRET\n");
	} else if (mode == 1) {
		// NATIVE_CODE mode is disabled
		print("Error: NATIVE_CODE mode is disabled. Use INTERPRET mode (0) instead.\n");
		return Variant(false);
	} else {
		print("Error: Invalid mode (0=INTERPRET, 1=NATIVE_CODE is disabled)\n");
		return Variant(false);
	}
	return Variant(true);
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
	ADD_API_FUNCTION(set_compiler_mode, "bool", "int mode", 
	                 "Set compiler mode (0=INTERPRET, 1=NATIVE_CODE)");
	ADD_API_FUNCTION(test_compile, "Variant", "", 
	                 "Test the compilation system with a simple function");
	ADD_API_FUNCTION(test_dataset, "Variant", "int count", 
	                 "Test compilation with entries from the GDScript dataset (count: 1-100, default: 10)");
	ADD_API_FUNCTION(get_random_test, "String", "", 
	                 "Get a random GDScript code sample from the dataset");
	
	halt();
}
