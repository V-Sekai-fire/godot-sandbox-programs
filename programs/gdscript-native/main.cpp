#include <api.hpp>
#include <cstring>
#include <sys/mman.h>
#include <memory>
#include <sstream>

// Direct AST to RISC-V emitter using biscuit (following BEAM JIT pattern)
#include "ast_to_riscv_biscuit.h"
#include "test_data_loader.h"
#include "parser/gdscript_parser.h"

using AsmCallback = Variant(*)();

// Compile a simple GDScript-like function to RISC-V and return a callable
static Variant compile_gdscript(String gdscript_code) {
	// Parse GDScript code to AST
	gdscript::GDScriptParser parser;
	if (!parser.isValid()) {
		print("Error: Parser initialization failed\n");
		return Nil;
	}
	
	std::string source = gdscript_code.utf8().get_data();
	auto ast = parser.parse(source);
	
	if (!ast) {
		print("Error: Failed to parse GDScript code\n");
		print("Error message: ", parser.getErrorMessage().c_str(), "\n");
		return Nil;
	}
	
	// Convert AST directly to RISC-V machine code using biscuit (like BEAM JIT with AsmJit)
	gdscript::ASTToRISCVEmitter emitter;
	auto [machineCode, codeSize] = emitter.emit(ast.get());
	
	if (machineCode == nullptr || codeSize == 0) {
		print("Error: Failed to emit RISC-V machine code\n");
		return Nil;
	}
	
	print("Successfully parsed GDScript code with ", ast->functions.size(), " function(s)\n");
	if (!ast->functions.empty()) {
		print("First function: ", ast->functions[0]->name.c_str(), "\n");
	}
	print("Generated ", codeSize, " bytes of RISC-V machine code\n");
	
	// Allocate executable memory and copy machine code (like BEAM JIT)
	void *executable = mmap(nullptr, codeSize, PROT_READ | PROT_WRITE | PROT_EXEC, 
	                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	
	if (executable == MAP_FAILED) {
		print("Error: Failed to allocate executable memory\n");
		return Nil;
	}
	
	std::memcpy(executable, machineCode, codeSize);
	AsmCallback callback = (AsmCallback)executable;
	return Callable::Create<Variant()>(callback);
}

// Test function: compile and call a simple function
static Variant test_compile() {
	print("Testing GDScript to RISC-V compilation...\n");
	
	// Create a simple function: func hello() -> int: return 42
	String testCode = "func hello() -> int:\n    return 42\n";
	
	Variant callable = compile_gdscript(testCode);
	
	if (callable.get_type() == Variant::Type::CALLABLE) {
		print("Compilation successful! Calling function...\n");
		Callable c = callable;
		Variant result = c.call();
		print("Result: ", result, "\n");
		return result;
	} else {
		print("Compilation failed\n");
		return Nil;
	}
}

// Load and test with dataset entries
static Variant test_dataset(int count) {
	print("Loading GDScript dataset...\n");
	
	gdscript::TestDataLoader loader;
	std::string datasetPath = "test_data/data/godot_dodo_4x_60k/godot_dodo_4x_60k_data.json";
	
	if (!loader.loadDataset(datasetPath)) {
		print("Error: Failed to load dataset from: ", datasetPath.c_str(), "\n");
		return Nil;
	}
	
	print("Loaded ", loader.getEntryCount(), " entries from dataset\n");
	
	// Get a subset of entries to test
	size_t testCount = (count > 0 && count < 100) ? count : 10;
	auto testEntries = loader.getSubset(testCount);
	
	print("Testing compilation with ", testEntries.size(), " entries...\n");
	
	int successCount = 0;
	int failCount = 0;
	
	for (size_t i = 0; i < testEntries.size(); ++i) {
		const auto* entry = testEntries[i];
		if (!entry) continue;
		
		print("Test ", (i + 1), "/", testEntries.size(), ": ", entry->instruction.c_str(), "\n");
		
		// Try to compile the GDScript code
		String gdscriptCode = String(entry->gdscript_code.c_str());
		Variant callable = compile_gdscript(gdscriptCode);
		
		if (callable.get_type() == Variant::Type::CALLABLE) {
			successCount++;
			print("  ✓ Compilation successful\n");
		} else {
			failCount++;
			print("  ✗ Compilation failed\n");
		}
	}
	
	print("Results: ", successCount, " successful, ", failCount, " failed\n");
	
	return successCount;
}

// Get a random entry from the dataset
static Variant get_random_test() {
	gdscript::TestDataLoader loader;
	std::string datasetPath = "test_data/data/godot_dodo_4x_60k/godot_dodo_4x_60k_data.json";
	
	if (!loader.loadDataset(datasetPath)) {
		return Nil;
	}
	
	const auto* entry = loader.getRandomEntry();
	if (!entry) {
		return Nil;
	}
	
	// Return the GDScript code as a string
	return String(entry->gdscript_code.c_str());
}

int main() {
	print("GDScript to RISC-V Compiler (direct AST to RISC-V, no MLIR)\n");
	
	// Add public API
	ADD_API_FUNCTION(compile_gdscript, "Callable", "String gdscript_code", 
	                 "Compile GDScript to RISC-V and return a callable function");
	ADD_API_FUNCTION(test_compile, "Variant", "", 
	                 "Test the compilation system with a simple function");
	ADD_API_FUNCTION(test_dataset, "Variant", "int count", 
	                 "Test compilation with entries from the GDScript dataset (count: 1-100, default: 10)");
	ADD_API_FUNCTION(get_random_test, "String", "", 
	                 "Get a random GDScript code sample from the dataset");
	
	halt();
}
