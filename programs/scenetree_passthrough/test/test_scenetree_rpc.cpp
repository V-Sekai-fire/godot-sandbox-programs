// Test program for scenetree_passthrough JSON-RPC server
// This tests basic connectivity and method calls

#include <iostream>
#include <string>
#include <sstream>
#include <curl/curl.h>

// Simple HTTP client for testing
std::string http_post_json(const std::string& url, const std::string& json_body) {
    CURL* curl;
    CURLcode res;
    std::string response;
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    
    if (curl) {
        struct MemoryStruct {
            char* memory;
            size_t size;
        };
        
        static MemoryStruct chunk;
        chunk.memory = new char[1];
        chunk.size = 0;
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, 
            curl_slist_append(nullptr, "Content-Type: application/json"));
        
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](char* contents, size_t size, size_t nmemb, void* userp) {
            auto* mem = static_cast<MemoryStruct*>(userp);
            char* ptr = realloc(mem->memory, mem->size + size * nmemb + 1);
            if (!ptr) {
                free(mem->memory);
                return 0;
            }
            mem->memory = ptr;
            memcpy(&mem->memory[mem->size], contents, size * nmemb);
            mem->size += size * nmemb;
            mem->memory[mem->size] = 0;
            return size * nmemb;
        });
        
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
        
        res = curl_easy_perform(curl);
        
        if (res == CURLE_OK) {
            response = chunk.memory;
        }
        
        free(chunk.memory);
        curl_slist_free_all(nullptr);
        curl_easy_cleanup(curl);
    }
    
    curl_global_cleanup();
    return response;
}

void test_method(const std::string& name, const std::string& method, const std::string& params = "[]") {
    std::cout << "Testing: " << name << " ... ";
    
    std::string json_request = R"({
        "jsonrpc": "2.0",
        "method": ")" + method + R"(",
        "params": ) + params + R"(,
        "id": 1
    })";
    
    std::string response = http_post_json("http://127.0.0.1:7777", json_request);
    
    if (response.find("\"jsonrpc\":\"2.0\"") != std::string::npos || 
        response.find("\"jsonrpc\": \"2.0\"") != std::string::npos) {
        std::cout << "PASS" << std::endl;
        std::cout << "  Response: " << response << std::endl;
    } else {
        std::cout << "FAIL" << std::endl;
        std::cout << "  Response: " << response << std::endl;
    }
}

int main() {
    std::cout << "=== SceneTree Passthrough JSON-RPC Tests ===" << std::endl;
    std::cout << std::endl;
    std::cout << "Waiting for server on 127.0.0.1:7777..." << std::endl;
    std::cout << "Press Enter when server is ready..." << std::endl;
    std::cin.get();
    
    int passed = 0, failed = 0;
    
    // Test 1: get_root
    try {
        std::cout << "Test 1: get_root ... ";
        std::string response = http_post_json("http://127.0.0.1:7777", 
            R"({"jsonrpc":"2.0","method":"get_root","params":[],"id":1})");
        if (response.find("jsonrpc") != std::string::npos) {
            std::cout << "PASS" << std::endl;
            passed++;
        } else {
            std::cout << "FAIL" << std::endl;
            std::cout << "  Response: " << response << std::endl;
            failed++;
        }
    } catch (...) {
        std::cout << "FAIL (exception)" << std::endl;
        failed++;
    }
    
    // Test 2: get_frame
    try {
        std::cout << "Test 2: get_frame ... ";
        std::string response = http_post_json("http://127.0.0.1:7777",
            R"({"jsonrpc":"2.0","method":"get_frame","params":[],"id":2})");
        if (response.find("jsonrpc") != std::string::npos) {
            std::cout << "PASS" << std::endl;
            passed++;
        } else {
            std::cout << "FAIL" << std::endl;
            std::cout << "  Response: " << response << std::endl;
            failed++;
        }
    } catch (...) {
        std::cout << "FAIL (exception)" << std::endl;
        failed++;
    }
    
    // Test 3: get_current_scene
    try {
        std::cout << "Test 3: get_current_scene ... ";
        std::string response = http_post_json("http://127.0.0.1:7777",
            R"({"jsonrpc":"2.0","method":"get_current_scene","params":[],"id":3})");
        if (response.find("jsonrpc") != std::string::npos) {
            std::cout << "PASS" << std::endl;
            passed++;
        } else {
            std::cout << "FAIL" << std::endl;
            std::cout << "  Response: " << response << std::endl;
            failed++;
        }
    } catch (...) {
        std::cout << "FAIL (exception)" << std::endl;
        failed++;
    }
    
    // Test 4: Invalid method (should return error)
    try {
        std::cout << "Test 4: Invalid method (expect error) ... ";
        std::string response = http_post_json("http://127.0.0.1:7777",
            R"({"jsonrpc":"2.0","method":"nonexistent_method","params":[],"id":4})");
        if (response.find("error") != std::string::npos && response.find("jsonrpc") != std::string::npos) {
            std::cout << "PASS" << std::endl;
            passed++;
        } else {
            std::cout << "FAIL" << std::endl;
            std::cout << "  Response: " << response << std::endl;
            failed++;
        }
    } catch (...) {
        std::cout << "FAIL (exception)" << std::endl;
        failed++;
    }
    
    std::cout << std::endl;
    std::cout << "===========================================" << std::endl;
    std::cout << "Results: " << passed << " passed, " << failed << " failed" << std::endl;
    std::cout << "===========================================" << std::endl;
    
    return failed > 0 ? 1 : 0;
}
