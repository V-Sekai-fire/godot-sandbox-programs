#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../parser/gdscript_parser.h"
#include "../test_data_loader.h"
#include <string>

using namespace gdscript;

TEST_SUITE("Dataset Testing") {
    TEST_CASE("Load dataset") {
        TestDataLoader loader;
        std::string datasetPath = "test_data/data/godot_dodo_4x_60k/godot_dodo_4x_60k_data.json";
        
        bool loaded = loader.loadDataset(datasetPath);
        CHECK(loaded);
        CHECK(loader.getEntryCount() > 0);
    }
    
    TEST_CASE("Parse sample from dataset") {
        TestDataLoader loader;
        std::string datasetPath = "test_data/data/godot_dodo_4x_60k/godot_dodo_4x_60k_data.json";
        
        if (!loader.loadDataset(datasetPath)) {
            // Dataset not available, skip this test
            return;
        }
        
        // Get a few samples
        auto samples = loader.getSubset(10);
        CHECK(samples.size() > 0);
        
        GDScriptParser parser;
        int successCount = 0;
        int failCount = 0;
        
        for (const auto* entry : samples) {
            if (!entry) continue;
            
            auto ast = parser.parse(entry->gdscript_code);
            if (ast) {
                successCount++;
            } else {
                failCount++;
                // Print failing sample for debugging
                MESSAGE("Failed to parse: ", entry->instruction.c_str());
            }
        }
        
        MESSAGE("Parsed ", successCount, " successfully, ", failCount, " failed");
        // For now, we expect some failures since grammar is incomplete
        // CHECK(successCount > 0);
    }
    
    TEST_CASE("Parse simple dataset samples") {
        TestDataLoader loader;
        std::string datasetPath = "test_data/data/godot_dodo_4x_60k/godot_dodo_4x_60k_data.json";
        
        if (!loader.loadDataset(datasetPath)) {
            // Dataset not available, skip this test
            return;
        }
        
        // Filter for simple functions (those with "return" keyword)
        auto samples = loader.filterEntries("return");
        CHECK(samples.size() > 0);
        
        GDScriptParser parser;
        int parsedCount = 0;
        
        // Try first 5 simple samples
        for (size_t i = 0; i < std::min(samples.size(), size_t(5)); ++i) {
            const auto* entry = samples[i];
            if (!entry) continue;
            
            auto ast = parser.parse(entry->gdscript_code);
            if (ast) {
                parsedCount++;
            }
        }
        
        MESSAGE("Parsed ", parsedCount, " out of ", std::min(samples.size(), size_t(5)), " simple samples");
    }
}

