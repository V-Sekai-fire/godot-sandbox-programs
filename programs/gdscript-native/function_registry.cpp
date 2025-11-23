#include "function_registry.h"
#include <vector>
#include <algorithm>

namespace gdscript {

void FunctionRegistry::registerFunction(const std::string& name, void* address, size_t size) {
    functions[name] = address;
    functionSizes[name] = size;
}

void* FunctionRegistry::getFunction(const std::string& name) const {
    auto it = functions.find(name);
    if (it != functions.end()) {
        return it->second;
    }
    return nullptr;
}

bool FunctionRegistry::hasFunction(const std::string& name) const {
    return functions.find(name) != functions.end();
}

std::vector<std::string> FunctionRegistry::getFunctionNames() const {
    std::vector<std::string> names;
    names.reserve(functions.size());
    for (const auto& pair : functions) {
        names.push_back(pair.first);
    }
    return names;
}

void FunctionRegistry::clear() {
    functions.clear();
    functionSizes.clear();
}

size_t FunctionRegistry::getFunctionSize(const std::string& name) const {
    auto it = functionSizes.find(name);
    if (it != functionSizes.end()) {
        return it->second;
    }
    return 0;
}

int64_t callAssemblyFunction(void* funcAddr) {
    // Cast to function pointer that returns int64_t
    // The generated assembly follows RISC-V 64 Linux ABI:
    // - Returns int64_t in register a0
    // - Takes no parameters (for now)
    using FuncPtr = int64_t(*)();
    FuncPtr func = reinterpret_cast<FuncPtr>(funcAddr);
    
    // Call the function
    return func();
}

} // namespace gdscript

