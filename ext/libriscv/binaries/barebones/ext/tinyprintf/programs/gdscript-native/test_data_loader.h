#pragma once

#include <string>
#include <vector>
#include <memory>

namespace gdscript {

// Test data entry from the dataset
struct TestDataEntry {
    std::string instruction;
    std::string gdscript_code;
    std::string description;
};

// Loader for GDScript test dataset
class TestDataLoader {
private:
    std::vector<TestDataEntry> entries;
    size_t currentIndex;

public:
    TestDataLoader();
    
    // Load dataset from JSON file
    bool loadDataset(const std::string& jsonPath);
    
    // Get a random entry
    const TestDataEntry* getRandomEntry() const;
    
    // Get entry by index
    const TestDataEntry* getEntry(size_t index) const;
    
    // Get total number of entries
    size_t getEntryCount() const { return entries.size(); }
    
    // Get a subset of entries for testing
    std::vector<const TestDataEntry*> getSubset(size_t count) const;
    
    // Get entries that match a filter (simple substring match)
    std::vector<const TestDataEntry*> filterEntries(const std::string& keyword) const;
};

} // namespace gdscript

