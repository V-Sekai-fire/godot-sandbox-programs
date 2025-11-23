#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

// Include all test files
#include "test_parser_basic.cpp"
#include "test_parser_expressions.cpp"
#include "test_parser_statements.cpp"
#include "test_compiler_basic.cpp"
#include "test_compiler_operators.cpp"
#include "test_compiler_system.cpp"
#include "test_compiler_codegen.cpp"
#include "test_compiler_integration.cpp"
#include "test_compiler_features.cpp"
#include "test_compiler_execution.cpp"
#include "test_end_to_end_features.cpp"
#include "test_ast_interpreter.cpp"
#include "test_adhoc.cpp"
#include "test_assignment_parse.cpp"
#include "test_token_debug.cpp"

