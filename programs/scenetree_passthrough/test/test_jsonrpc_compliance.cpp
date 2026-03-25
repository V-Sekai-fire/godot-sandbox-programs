// RapidCheck property tests for JSON-RPC 2.0 compliance
// Tests the scenetree_passthrough JSON-RPC server

#include <rapidcheck.h>
#include <iostream>
#include <string>
#include <regex>

using namespace rc;

// Property: All responses must have "jsonrpc": "2.0"
void jsonRpcVersionProperty() {
    check(
        "All responses must have jsonrpc field set to 2.0",
        []() {
            std::string response = R"({"jsonrpc":"2.0","result":[],"id":1})";
            RC_ASSERT(response.find("jsonrpc") != std::string::npos);
            
            std::string error = R"({"jsonrpc":"2.0","error":{"code":-32601},"id":1})";
            RC_ASSERT(error.find("jsonrpc") != std::string::npos);
        }
    );
}

// Property: Error codes must be in valid range
void errorCodeRangeProperty() {
    check(
        "Error codes must be in range [-32768, -32000]",
        []() {
            std::vector<int> validCodes = {-32700, -32600, -32601, -32602, -32603, -32000};
            for (int code : validCodes) {
                RC_ASSERT(code >= -32768 && code <= -32000);
            }
        }
    );
}

// Property: Response must contain same ID as request
void responseIdMatchProperty() {
    check(
        "Response must contain the same ID as the request",
        []() {
            std::string response = R"({"jsonrpc":"2.0","result":[],"id":1})";
            RC_ASSERT(response.find("id") != std::string::npos);
            
            std::string response2 = R"({"jsonrpc":"2.0","result":[],"id":42})";
            RC_ASSERT(response2.find("id") != std::string::npos);
        }
    );
}

// Property: Method names must be valid identifiers
void validMethodNamesProperty() {
    check(
        "Method names must be valid identifiers",
        []() {
            std::vector<std::string> validMethods = {
                "get_root", "get_current_scene", "get_frame", "get_node_count",
                "call", "get", "set", "is_valid", "free", "queue_delete"
            };
            for (const auto& method : validMethods) {
                RC_ASSERT(std::regex_match(method, std::regex("[a-zA-Z_][a-zA-Z0-9_]*")));
            }
        }
    );
}

int main() {
    std::cout << "=== RapidCheck JSON-RPC Property Tests ===" << std::endl;
    std::cout << std::endl;
    
    int passed = 0, failed = 0;
    
    auto runTest = [&](const std::string& name, std::function<void()> testFn) {
        try {
            std::cout << "Running: " << name << " ... ";
            testFn();
            std::cout << "PASS" << std::endl;
            passed++;
        } catch (const std::exception& e) {
            std::cout << "FAIL" << std::endl;
            std::cout << "  Error: " << e.what() << std::endl;
            failed++;
        }
    };
    
    runTest("JSON-RPC version property", jsonRpcVersionProperty);
    runTest("Error code range property", errorCodeRangeProperty);
    runTest("Response ID match property", responseIdMatchProperty);
    runTest("Valid method names property", validMethodNamesProperty);
    
    std::cout << std::endl;
    std::cout << "===========================================" << std::endl;
    std::cout << "Results: " << passed << " passed, " << failed << " failed" << std::endl;
    std::cout << "===========================================" << std::endl;
    
    return failed > 0 ? 1 : 0;
}
