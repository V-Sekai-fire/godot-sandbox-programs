#pragma once

#include <cstddef>
#include <vector>
#include <sys/mman.h>
#include <cstring>

namespace gdscript {

// RAII wrapper for executable memory allocated with mmap
// Automatically frees memory on destruction
class ExecutableMemory {
private:
    void* ptr;
    size_t size;
    bool valid;

public:
    // Allocate executable memory
    ExecutableMemory(size_t size);
    
    // Non-copyable
    ExecutableMemory(const ExecutableMemory&) = delete;
    ExecutableMemory& operator=(const ExecutableMemory&) = delete;
    
    // Movable
    ExecutableMemory(ExecutableMemory&& other) noexcept;
    ExecutableMemory& operator=(ExecutableMemory&& other) noexcept;
    
    // Free memory on destruction
    ~ExecutableMemory();
    
    // Get pointer to executable memory
    void* get() const { return ptr; }
    
    // Get size
    size_t getSize() const { return size; }
    
    // Check if valid
    bool isValid() const { return valid; }
    
    // Copy data into executable memory
    void copy(const void* data, size_t dataSize);
    
    // Release ownership (caller responsible for cleanup)
    void* release();
};

// Memory manager to track all executable memory allocations
class CodeMemoryManager {
private:
    std::vector<std::unique_ptr<ExecutableMemory>> allocations;
    
public:
    CodeMemoryManager() = default;
    ~CodeMemoryManager() = default;
    
    // Allocate and track executable memory
    // Returns pointer to ExecutableMemory (managed by this class)
    ExecutableMemory* allocate(size_t size);
    
    // Get count of tracked allocations
    size_t getAllocationCount() const { return allocations.size(); }
    
    // Clear all tracked allocations (they will be freed automatically)
    void clear();
};

} // namespace gdscript

