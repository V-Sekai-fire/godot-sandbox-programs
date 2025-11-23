#include "doctest.h"
#include "test_compiler_helpers.h"

TEST_SUITE("Compiler - Basic Function Compilation (from godot-dodo corpus)") {
    TEST_CASE("Return the default import order for the script") {
        std::string source = R"(func _get_import_order() -> int:
	return IMPORT_ORDER_DEFAULT
)";
        auto result = compileGDScript(source);
        
        if (!result.success && result.ast) {
            // Debug: Print AST if parsing succeeded but compilation failed
            print_ast_message(result.ast.get(), "AST (parse succeeded, compilation failed)");
        } else if (!result.ast) {
            MESSAGE("Parse failed. Error: " << result.errorMessage);
        }
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        if (result.success) {
            CHECK(result.codeSize > 0);
        }
    }
    
    TEST_CASE("Return the string BlendBurn") {
        std::string source = R"(func _get_name():
	return "BlendBurn"
)";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        if (result.success) {
            CHECK(result.codeSize > 0);
        }
    }
    
    TEST_CASE("Return boolean indicating if object is navigation controller") {
        std::string source = R"(func is_navigation_controller() -> bool:
	return false
)";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        if (result.success) {
            CHECK(result.codeSize > 0);
        }
    }
    
    TEST_CASE("Return music as shortcode") {
        std::string source = R"(func get_shortcode() -> String:
	return "music"
)";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        if (result.success) {
            CHECK(result.codeSize > 0);
        }
    }
    
    TEST_CASE("Return null as NavigationMesh template") {
        std::string source = R"(func _get_navmesh_template() -> NavigationMesh :
	return null
)";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        if (result.success) {
            CHECK(result.codeSize > 0);
        }
    }
    
    TEST_CASE("Return OptionIndex variable") {
        std::string source = R"(func get_option_index():
	return OptionIndex
	
)";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        if (result.success) {
            CHECK(result.codeSize > 0);
        }
    }
    
    TEST_CASE("Return private variable _path") {
        std::string source = R"(func get_path():
		return _path
)";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        if (result.success) {
            CHECK(result.codeSize > 0);
        }
    }
    
    TEST_CASE("Return boolean for history advance block") {
        std::string source = R"(func history_advance_block() -> bool:
	return is_mouse_on_button or is_history_open 
)";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        if (result.success) {
            CHECK(result.codeSize > 0);
        }
    }
    
    TEST_CASE("Return cell_size as Vector2") {
        std::string source = R"(func get_cell_size() -> Vector2:
	return (cell_size)
)";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        if (result.success) {
            CHECK(result.codeSize > 0);
        }
    }
}
