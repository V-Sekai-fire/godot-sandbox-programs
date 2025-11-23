#ifndef TEST_COMPILER_HELPERS_H
#define TEST_COMPILER_HELPERS_H

#include "../src/parser/gdscript_parser.h"
#include "../src/ast_to_riscv_biscuit.h"
#include <string>
#include <memory>
#include <vector>
#include <cstring>
#include <sys/mman.h>

using namespace gdscript;

// Helper to compile GDScript and get machine code
struct CompilationResult {
    bool success;
    std::unique_ptr<ProgramNode> ast;
    std::vector<uint8_t> code;  // Own the code buffer to keep it alive
    size_t codeSize;
    std::string errorMessage;
    
    // Get pointer to code for execution
    const uint8_t* get_code_ptr() const { return code.data(); }
};

inline CompilationResult compileGDScript(const std::string& source) {
    CompilationResult result;
    result.success = false;
    result.codeSize = 0;
    
    // Parse
    GDScriptParser parser;
    if (!parser.is_valid()) {
        result.errorMessage = "Parser initialization failed";
        return result;
    }
    
    result.ast = parser.parse(source);
    if (!result.ast) {
        result.errorMessage = parser.getErrorMessage();
        return result;
    }
    
    // Emit RISC-V code
    ASTToRISCVEmitter emitter;
    auto [code, size] = emitter.emit(result.ast.get());
    
    if (code == nullptr || size == 0) {
        result.errorMessage = "Code generation failed";
        return result;
    }
    
    // Copy code to our own buffer (emitter's buffer will go out of scope)
    result.code.resize(size);
    std::memcpy(result.code.data(), code, size);
    result.codeSize = size;
    result.success = true;
    return result;
}

// Helper to execute generated RISC-V code and get result
inline int64_t execute_generated_code(const uint8_t* code, size_t size) {
    if (code == nullptr || size == 0) {
        return 0;
    }
    
    // Allocate executable memory with mmap
    void* execMem = mmap(nullptr, size, PROT_READ | PROT_WRITE | PROT_EXEC, 
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (execMem == MAP_FAILED) {
        return 0;
    }
    
    // Copy code to executable memory
    std::memcpy(execMem, code, size);
    
    // Cast to function pointer and call
    using FuncPtr = int64_t(*)();
    FuncPtr func = reinterpret_cast<FuncPtr>(execMem);
    int64_t result = func();
    
    // Cleanup
    munmap(execMem, size);
    
    return result;
}

#endif // TEST_COMPILER_HELPERS_H

