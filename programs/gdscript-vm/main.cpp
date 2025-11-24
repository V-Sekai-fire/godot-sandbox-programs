#include <api.hpp>
#include <memory>
#include <string>

// Port of Godot's GDScript VM to godot-sandbox
// This program provides GDScript bytecode execution using Godot's VM

// Include our parser from gdscript-native
#include "../gdscript-native/src/parser/gdscript_parser.h"
#include "../gdscript-native/src/parser/ast.h"

// Include Godot VM components
#include "gdscript.h"
#include "gdscript_function.h"
#include "ast_to_bytecode_adapter.h"

using namespace gdscript;

// Create a minimal GDScript instance stub for compilation
// This is needed by the bytecode generator
class MinimalGDScript : public GDScript {
public:
    MinimalGDScript() : GDScript() {
        valid = true; // Mark as valid for compilation
    }
    ~MinimalGDScript() {}
};

static Variant compile_and_execute_gdscript(String gdscript_code) {
    // Step 1: Parse GDScript to AST using our parser
    GDScriptParser parser;
    if (!parser.is_valid()) {
        print("Error: Parser initialization failed\n");
        return Variant(0);
    }
    
    std::string source = gdscript_code.utf8().get_data();
    std::unique_ptr<ProgramNode> ast = parser.parse(source);
    
    if (!ast) {
        const ErrorCollection& errors = parser.get_errors();
        if (errors.has_errors()) {
            std::string error_msg = errors.get_formatted_message();
            print("Error: Failed to parse GDScript code\n");
            print("Error details: ", error_msg.c_str(), "\n");
        }
        return Variant(0);
    }
    
    // Step 2: Convert AST to bytecode using adapter
    ASTToBytecodeAdapter adapter;
    
    // Create minimal GDScript instance for compilation
    // Use RefCounted pattern - create on heap
    MinimalGDScript* script = new MinimalGDScript();
    Ref<GDScript> script_ref(script);
    
    // Compile each function individually
    HashMap<StringName, GDScriptFunction*> functions;
    for (const auto& func_node : ast->functions) {
        if (!func_node) continue;
        
        StringName func_name(func_node->name.c_str());
        GDScriptFunction* compiled_func = adapter.compile_function(func_node.get(), script);
        if (compiled_func) {
            functions[func_name] = compiled_func;
            // Add function to script's member_functions map
            script->member_functions[func_name] = compiled_func;
        }
    }
    
    if (functions.is_empty()) {
        print("Error: Failed to compile GDScript to bytecode\n");
        return Variant(0);
    }
    
    // Step 3: Execute bytecode via VM
    // Find the first function (or a function named "test" or "main")
    GDScriptFunction* func_to_call = nullptr;
    StringName test_name("test");
    StringName main_name("main");
    
    if (functions.has(test_name)) {
        func_to_call = functions[test_name];
    } else if (functions.has(main_name)) {
        func_to_call = functions[main_name];
    } else if (!functions.is_empty()) {
        // Get first function
        func_to_call = functions.begin()->value;
    }
    
    if (!func_to_call) {
        print("Error: No function found to execute\n");
        return Variant(0);
    }
    
    // Create a minimal GDScriptInstance for execution
    // GDScriptInstance requires an Object owner - create a minimal stub
    class MinimalObject : public Object {
    public:
        MinimalObject() : Object() {}
        ~MinimalObject() {}
    };
    
    MinimalObject owner;
    GDScriptInstance instance;
    instance.owner = &owner;
    instance.script = script_ref;
    
    // Execute function
    Callable::CallError call_error;
    Variant result = func_to_call->call(&instance, nullptr, 0, call_error);
    
    if (call_error.error != Callable::CallError::CALL_OK) {
        print("Error: Function call failed with error code: ", call_error.error, "\n");
        return Variant(0);
    }
    
    return result;
}

static Variant test_vm() {
    String test_code = R"(func test():
    return 42
)";
    return compile_and_execute_gdscript(test_code);
}

int main() {
    print("GDScript VM program initialized");
    
    // Add public API
    ADD_API_FUNCTION(compile_and_execute_gdscript, "Variant", "String gdscript_code", 
                     "Compile and execute GDScript code using Godot's bytecode VM");
    ADD_API_FUNCTION(test_vm, "Variant", "", "Test the VM with a simple function");
    
    halt();
}

