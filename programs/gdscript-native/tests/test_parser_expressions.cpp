#include "doctest.h"
#include "../src/parser/gdscript_parser.h"
#include "../src/parser/ast.h"
#include <string>
#include <memory>

using namespace gdscript;

TEST_SUITE("Literal Parsing (from godot-dodo corpus)") {
    TEST_CASE("Parse function returning integer constant") {
        GDScriptParser parser;
        std::string source = R"(func _get_import_order() -> int:
	return IMPORT_ORDER_DEFAULT
)";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
    }
    
    TEST_CASE("Parse function returning string literal") {
        GDScriptParser parser;
        std::string source = R"(func _get_name():
	return "BlendBurn"
)";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
    }
    
    TEST_CASE("Parse function returning boolean false") {
        GDScriptParser parser;
        std::string source = R"(func is_navigation_controller() -> bool:
	return false
)";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
    }
    
    TEST_CASE("Parse function returning string music") {
        GDScriptParser parser;
        std::string source = R"(func get_shortcode() -> String:
	return "music"
)";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
    }
    
    TEST_CASE("Parse function returning null") {
        GDScriptParser parser;
        std::string source = R"(func _get_navmesh_template() -> NavigationMesh :
	return null
)";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
    }
}

TEST_SUITE("Identifier Parsing (from godot-dodo corpus)") {
    TEST_CASE("Parse function returning identifier OptionIndex") {
        GDScriptParser parser;
        std::string source = R"(func get_option_index():
	return OptionIndex
	
)";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
    }
}

TEST_SUITE("Binary Operations (from godot-dodo corpus)") {
    TEST_CASE("Parse function with comparison operation") {
        GDScriptParser parser;
        std::string source = R"(func is_not_null(actual_value, marker = null):
	is_true(actual_value != null, marker)
	return actual_value
)";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
    }
    
    TEST_CASE("Parse function with assignment operation") {
        GDScriptParser parser;
        std::string source = R"(func reset_typing() -> void:
	char_idx = -1
)";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
    }
    
    TEST_CASE("Parse function with parameter assignment") {
        GDScriptParser parser;
        std::string source = R"(func _import_option_toggled(pressed: bool) -> void:
	use_imported_size = pressed
	
)";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
    }
}
