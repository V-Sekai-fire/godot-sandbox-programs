#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace gdscript {

// Function registry to track generated function addresses
// Used for creating C++ wrappers that convert int64 to Variant
class FunctionRegistry {
private:
    // Map function name to executable memory address
    std::unordered_map<std::string, void*> _functions;
    
    // Map function name to code size (for cleanup)
    std::unordered_map<std::string, size_t> _function_sizes;

public:
    FunctionRegistry() = default;
    ~FunctionRegistry() = default;
    
    /// \brief Register a compiled function
    /// \param name Function name
    /// \param address Pointer to executable memory containing the function
    /// \param size Size of the code in bytes (for tracking)
    void register_function(const std::string& name, void* address, size_t size);
    
    /// \brief Get function address by name
    /// \param name Function name to look up
    /// \return Pointer to executable memory, or nullptr if not found
    void* get_function(const std::string& name) const;
    
    /// \brief Check if function is registered
    /// \param name Function name to check
    /// \return True if function is registered, false otherwise
    bool has_function(const std::string& name) const;
    
    /// \brief Get all registered function names
    /// \return Vector of all registered function names
    std::vector<std::string> get_function_names() const;
    
    /// \brief Clear all registered functions (does not free memory)
    /// \note This only clears the registry, it does not free the executable memory
    void clear();
    
    /// \brief Get function size in bytes
    /// \param name Function name
    /// \return Size in bytes, or 0 if function not found
    size_t get_function_size(const std::string& name) const;
};

// Helper function to call assembly function and get int64 result
// func_addr: Pointer to executable memory containing the function
// Returns: int64_t result from the function
int64_t call_assembly_function(void* func_addr);

} // namespace gdscript

