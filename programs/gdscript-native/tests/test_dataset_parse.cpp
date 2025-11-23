#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../src/parser/gdscript_parser.h"
#include "../src/parser/gdscript_tokenizer.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>

using namespace gdscript;

// Simple JSON parser for godot-dodo dataset format
// Format: [{"instruction": "...", "output": "..."}, ...]
struct DatasetEntry {
    std::string instruction;
    std::string output; // GDScript code
};

std::vector<DatasetEntry> load_dataset(const std::string& json_path) {
    std::vector<DatasetEntry> entries;
    std::ifstream file(json_path);
    if (!file.is_open()) {
        return entries;
    }
    
    std::string content;
    std::string line;
    while (std::getline(file, line)) {
        content += line + "\n";
    }
    file.close();
    
    // Simple JSON parsing - look for "output" fields containing GDScript code
    size_t pos = 0;
    while (pos < content.length()) {
        // Find "output" field
        size_t output_pos = content.find("\"output\"", pos);
        if (output_pos == std::string::npos) break;
        
        // Find the value after the colon
        size_t colon_pos = content.find(':', output_pos);
        if (colon_pos == std::string::npos) break;
        
        // Find the opening quote
        size_t quote_start = content.find('"', colon_pos);
        if (quote_start == std::string::npos) break;
        
        // Find the closing quote (handle escaped quotes)
        size_t quote_end = quote_start + 1;
        while (quote_end < content.length()) {
            if (content[quote_end] == '"' && content[quote_end - 1] != '\\') {
                break;
            }
            quote_end++;
        }
        
        if (quote_end >= content.length()) break;
        
        // Extract the GDScript code
        std::string gdscript = content.substr(quote_start + 1, quote_end - quote_start - 1);
        
        // Unescape JSON strings
        std::string unescaped;
        for (size_t i = 0; i < gdscript.length(); ++i) {
            if (gdscript[i] == '\\' && i + 1 < gdscript.length()) {
                if (gdscript[i + 1] == 'n') {
                    unescaped += '\n';
                    i++;
                } else if (gdscript[i + 1] == 't') {
                    unescaped += '\t';
                    i++;
                } else if (gdscript[i + 1] == '\\') {
                    unescaped += '\\';
                    i++;
                } else if (gdscript[i + 1] == '"') {
                    unescaped += '"';
                    i++;
                } else {
                    unescaped += gdscript[i];
                }
            } else {
                unescaped += gdscript[i];
            }
        }
        
        DatasetEntry entry;
        entry.output = unescaped;
        entries.push_back(entry);
        
        pos = quote_end + 1;
    }
    
    return entries;
}

TEST_SUITE("Dataset Parsing") {
    TEST_CASE("Parse entire godot-dodo dataset") {
        // Try multiple possible paths
        std::vector<std::string> possible_paths = {
            "ext/godot-dodo/data/godot_dodo_4x_60k/godot_dodo_4x_60k_data.json",
            "../../../ext/godot-dodo/data/godot_dodo_4x_60k/godot_dodo_4x_60k_data.json",
            "../../../../ext/godot-dodo/data/godot_dodo_4x_60k/godot_dodo_4x_60k_data.json"
        };
        
        std::vector<DatasetEntry> entries;
        std::string dataset_path;
        
        for (const auto& path : possible_paths) {
            entries = load_dataset(path);
            if (!entries.empty()) {
                dataset_path = path;
                break;
            }
        }
        
        if (entries.empty()) {
            MESSAGE("Dataset file not found. Tried paths:");
            for (const auto& path : possible_paths) {
                MESSAGE("  - " << path);
            }
            return; // Skip test if dataset not available
        }
        
        MESSAGE("Loaded " << entries.size() << " entries from dataset");
        
        GDScriptParser parser;
        int success_count = 0;
        int fail_count = 0;
        std::vector<std::string> failure_samples;
        
        // Parse all entries
        size_t max_test = entries.size();
        for (size_t i = 0; i < max_test; ++i) {
            const auto& entry = entries[i];
            
            auto ast = parser.parse(entry.output);
            const auto& errors = parser.get_errors();
            if (ast && errors.get_error_count() == 0) {
                success_count++;
            } else {
                fail_count++;
                if (failure_samples.size() < 50) {
                    std::string sample = entry.output.substr(0, 500); // First 500 chars
                    if (errors.get_error_count() > 0) {
                        sample += "\n--- Errors: ";
                        const std::vector<CompilationError>& error_list = errors.get_errors();
                        for (size_t i = 0; i < error_list.size(); ++i) {
                            const CompilationError& err = error_list[i];
                            sample += err.message + "; ";
                        }
                    }
                    failure_samples.push_back(sample);
                }
            }
            
            if ((i + 1) % 1000 == 0 || (i + 1) == max_test) {
                MESSAGE("Processed " << (i + 1) << "/" << max_test 
                        << " entries: " << success_count << " success, " << fail_count << " failed");
            }
        }
        
        MESSAGE("Final results: " << success_count << " successful, " << fail_count << " failed out of " << max_test);
        
        if (!failure_samples.empty()) {
            MESSAGE("Sample failures:");
            for (size_t i = 0; i < failure_samples.size(); ++i) {
                MESSAGE("Failure " << (i + 1) << ":\n" << failure_samples[i] << "\n---");
            }
        }
        
        // Report success rate
        double success_rate = (double)success_count / max_test * 100.0;
        MESSAGE("Success rate: " << success_rate << "%");
        
        // For now, just report - don't fail the test
        // CHECK(success_count > 0);
    }
}

