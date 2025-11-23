#include "test_data_loader.h"
#include <fstream>
#include <sstream>
#include <random>
#include <algorithm>

namespace gdscript {

TestDataLoader::TestDataLoader() : currentIndex(0) {}

bool TestDataLoader::loadDataset(const std::string& jsonPath) {
    entries.clear();
    
    std::ifstream file(jsonPath);
    if (!file.is_open()) {
        return false;
    }
    
    // Read entire file
    std::string jsonContent;
    std::string line;
    while (std::getline(file, line)) {
        jsonContent += line + "\n";
    }
    file.close();
    
    // Simple JSON parser for this specific format
    // Format: [{"instruction": "...", "output": "..."}, ...]
    // We'll parse entry by entry looking for the pattern
    
    size_t pos = 0;
    size_t entryCount = 0;
    const size_t maxEntries = 1000; // Limit for testing
    
    while (entryCount < maxEntries && (pos = jsonContent.find("\"instruction\"", pos)) != std::string::npos) {
        TestDataEntry entry;
        
        // Extract instruction - find the value after "instruction":
        size_t colonPos = jsonContent.find(":", pos);
        if (colonPos == std::string::npos) break;
        
        // Find opening quote
        size_t instStart = jsonContent.find("\"", colonPos);
        if (instStart == std::string::npos) break;
        instStart++; // Skip opening quote
        
        // Find closing quote (handle escaped quotes)
        size_t instEnd = instStart;
        while (instEnd < jsonContent.size()) {
            if (jsonContent[instEnd] == '"' && jsonContent[instEnd - 1] != '\\') {
                break;
            }
            instEnd++;
        }
        if (instEnd >= jsonContent.size()) break;
        
        entry.instruction = jsonContent.substr(instStart, instEnd - instStart);
        
        // Find output field
        size_t outPos = jsonContent.find("\"output\"", instEnd);
        if (outPos == std::string::npos) break;
        
        size_t outColonPos = jsonContent.find(":", outPos);
        if (outColonPos == std::string::npos) break;
        
        size_t outStart = jsonContent.find("\"", outColonPos);
        if (outStart == std::string::npos) break;
        outStart++; // Skip opening quote
        
        // Find closing quote for output (handle escaped quotes and newlines)
        size_t outEnd = outStart;
        while (outEnd < jsonContent.size()) {
            if (jsonContent[outEnd] == '"' && (outEnd == outStart || jsonContent[outEnd - 1] != '\\')) {
                break;
            }
            outEnd++;
        }
        if (outEnd >= jsonContent.size()) break;
        
        // Extract and unescape the GDScript code
        std::string rawOutput = jsonContent.substr(outStart, outEnd - outStart);
        
        // Unescape common escape sequences
        std::ostringstream oss;
        for (size_t i = 0; i < rawOutput.size(); ++i) {
            if (rawOutput[i] == '\\' && i + 1 < rawOutput.size()) {
                switch (rawOutput[i + 1]) {
                    case 'n': oss << '\n'; ++i; break;
                    case 't': oss << '\t'; ++i; break;
                    case 'r': oss << '\r'; ++i; break;
                    case '\\': oss << '\\'; ++i; break;
                    case '"': oss << '"'; ++i; break;
                    default: oss << rawOutput[i]; break;
                }
            } else {
                oss << rawOutput[i];
            }
        }
        entry.gdscript_code = oss.str();
        entry.description = entry.instruction;
        
        entries.push_back(entry);
        entryCount++;
        pos = outEnd;
    }
    
    return !entries.empty();
}

const TestDataEntry* TestDataLoader::getRandomEntry() const {
    if (entries.empty()) return nullptr;
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dis(0, entries.size() - 1);
    
    return &entries[dis(gen)];
}

const TestDataEntry* TestDataLoader::getEntry(size_t index) const {
    if (index >= entries.size()) return nullptr;
    return &entries[index];
}

std::vector<const TestDataEntry*> TestDataLoader::getSubset(size_t count) const {
    std::vector<const TestDataEntry*> result;
    count = std::min(count, entries.size());
    
    for (size_t i = 0; i < count; ++i) {
        result.push_back(&entries[i]);
    }
    
    return result;
}

std::vector<const TestDataEntry*> TestDataLoader::filterEntries(const std::string& keyword) const {
    std::vector<const TestDataEntry*> result;
    std::string lowerKeyword = keyword;
    std::transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(), ::tolower);
    
    for (const auto& entry : entries) {
        std::string lowerInst = entry.instruction;
        std::transform(lowerInst.begin(), lowerInst.end(), lowerInst.begin(), ::tolower);
        
        if (lowerInst.find(lowerKeyword) != std::string::npos) {
            result.push_back(&entry);
        }
    }
    
    return result;
}

} // namespace gdscript

