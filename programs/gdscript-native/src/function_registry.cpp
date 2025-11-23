#include "function_registry.h"
#include <vector>
#include <string>
#include <algorithm>

namespace gdscript {

void FunctionRegistry::register_function(const std::string& name, void* address, size_t size) {
    _functions[name] = address;
    _function_sizes[name] = size;
}

void* FunctionRegistry::get_function(const std::string& name) const {
    std::unordered_map<std::string, void*>::const_iterator it = _functions.find(name);
    if (it != _functions.end()) {
        return it->second;
    }
    return nullptr;
}

bool FunctionRegistry::has_function(const std::string& name) const {
    return _functions.find(name) != _functions.end();
}

std::vector<std::string> FunctionRegistry::get_function_names() const {
    std::vector<std::string> names;
    names.reserve(_functions.size());
    for (const std::pair<const std::string, void*>& pair : _functions) {
        names.push_back(pair.first);
    }
    return names;
}

void FunctionRegistry::clear() {
    _functions.clear();
    _function_sizes.clear();
}

size_t FunctionRegistry::get_function_size(const std::string& name) const {
    std::unordered_map<std::string, size_t>::const_iterator it = _function_sizes.find(name);
    if (it != _function_sizes.end()) {
        return it->second;
    }
    return 0;
}

int64_t call_assembly_function(void* func_addr) {
    // Cast to function pointer that returns int64_t
    // The generated assembly follows RISC-V 64 Linux ABI:
    // - Returns int64_t in register a0
    // - Takes no parameters (for now)
    using FuncPtr = int64_t(*)();
    FuncPtr func = reinterpret_cast<FuncPtr>(func_addr);
    
    // Call the function
    return func();
}

} // namespace gdscript

