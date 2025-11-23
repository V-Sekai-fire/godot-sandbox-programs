#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>

namespace gdscript {

// Function registry to track generated function addresses
// Used for creating C++ wrappers that convert int64 to Variant
class FunctionRegistry {
private:
    // Map function name to executable memory address
    std::unordered_map<std::string, void*> functions;
    
    // Map function name to code size (for cleanup)
    std::unordered_map<std::string, size_t> functionSizes;

public:
    FunctionRegistry() = default;
    ~FunctionRegistry() = default;
    
    // Register a compiled function
    // name: Function name
    // address: Pointer to executable memory
    // size: Size of the code (for tracking)
    void registerFunction(const std::string& name, void* address, size_t size);
    
    // Get function address by name
    // Returns nullptr if not found
    void* getFunction(const std::string& name) const;
    
    // Check if function is registered
    bool hasFunction(const std::string& name) const;
    
    // Get all registered function names
    std::vector<std::string> getFunctionNames() const;
    
    // Clear all registered functions (does not free memory)
    void clear();
    
    // Get function size
    size_t getFunctionSize(const std::string& name) const;
};

// Helper function to call assembly function and get int64 result
// funcAddr: Pointer to executable memory containing the function
// Returns: int64_t result from the function
int64_t callAssemblyFunction(void* funcAddr);

} // namespace gdscript

