#include "code_memory_manager.h"
#include <stdexcept>

namespace gdscript {

ExecutableMemory::ExecutableMemory(size_t size) : ptr(nullptr), size(size), valid(false) {
    if (size == 0) {
        return;
    }
    
    // Allocate executable memory with mmap
    ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE | PROT_EXEC, 
               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (ptr == MAP_FAILED) {
        ptr = nullptr;
        valid = false;
    } else {
        valid = true;
    }
}

ExecutableMemory::ExecutableMemory(ExecutableMemory&& other) noexcept
    : ptr(other.ptr), size(other.size), valid(other.valid) {
    other.ptr = nullptr;
    other.size = 0;
    other.valid = false;
}

ExecutableMemory& ExecutableMemory::operator=(ExecutableMemory&& other) noexcept {
    if (this != &other) {
        // Free current memory
        if (valid && ptr != nullptr) {
            munmap(ptr, size);
        }
        
        // Move from other
        ptr = other.ptr;
        size = other.size;
        valid = other.valid;
        
        // Clear other
        other.ptr = nullptr;
        other.size = 0;
        other.valid = false;
    }
    return *this;
}

ExecutableMemory::~ExecutableMemory() {
    if (valid && ptr != nullptr) {
        munmap(ptr, size);
    }
}

void ExecutableMemory::copy(const void* data, size_t dataSize) {
    if (!valid || ptr == nullptr) {
        return;
    }
    
    if (dataSize > size) {
        dataSize = size; // Clamp to available size
    }
    
    std::memcpy(ptr, data, dataSize);
}

void* ExecutableMemory::release() {
    void* released = ptr;
    ptr = nullptr;
    size = 0;
    valid = false;
    return released;
}

ExecutableMemory* CodeMemoryManager::allocate(size_t size) {
    std::unique_ptr<ExecutableMemory> mem = std::make_unique<ExecutableMemory>(size);
    ExecutableMemory* ptr = mem.get();
    allocations.push_back(std::move(mem));
    return ptr;
}

void CodeMemoryManager::clear() {
    allocations.clear();
}

} // namespace gdscript

