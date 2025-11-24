#include "doctest.h"
#include "../src/parser/gdscript_parser.h"
#include "../src/parser/ast.h"
#include <string>
#include <memory>

using namespace gdscript;

TEST_SUITE("Function Parsing (from godot-dodo corpus)") {
    TEST_CASE("Parse simple function with return type") {
        GDScriptParser parser;
        std::string source = R"(func _get_import_order() -> int:
	return IMPORT_ORDER_DEFAULT
)";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
        CHECK(ast->functions.size() >= 1);
    }
    
    TEST_CASE("Parse function without return type") {
        GDScriptParser parser;
        std::string source = R"(func _get_name():
	return "BlendBurn"
)";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
        CHECK(ast->functions.size() >= 1);
    }
    
    TEST_CASE("Parse function with void return type") {
        GDScriptParser parser;
        std::string source = R"(func reset_typing() -> void:
	char_idx = -1
)";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
        CHECK(ast->functions.size() >= 1);
    }
    
    TEST_CASE("Parse function with parameters") {
        GDScriptParser parser;
        std::string source = R"(func _import_option_toggled(pressed: bool) -> void:
	use_imported_size = pressed
	
)";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
        CHECK(ast->functions.size() >= 1);
    }
}

TEST_SUITE("Return Statement (from godot-dodo corpus)") {
    TEST_CASE("Parse return with identifier") {
        GDScriptParser parser;
        std::string source = R"(func get_option_index():
	return OptionIndex
	
)";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
    }
    
    TEST_CASE("Parse return with string literal") {
        GDScriptParser parser;
        std::string source = R"(func get_shortcode() -> String:
	return "music"
)";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
    }
    
    TEST_CASE("Parse return with boolean") {
        GDScriptParser parser;
        std::string source = R"(func is_navigation_controller() -> bool:
	return false
)";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
    }
    
    TEST_CASE("Parse return with null") {
        GDScriptParser parser;
        std::string source = R"(func _get_navmesh_template() -> NavigationMesh :
	return null
)";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
    }
}

TEST_SUITE("Variable Declaration (from godot-dodo corpus)") {
    TEST_CASE("Parse function with pass statement") {
        GDScriptParser parser;
        std::string source = R"(func only_one_time_call() -> void:
		pass
)";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
    }
}
