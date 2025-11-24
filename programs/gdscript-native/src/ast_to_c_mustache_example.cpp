// Example: How to use Mustache templates for C code generation
// This is an alternative approach - current implementation uses simple string building
// To use this, uncomment and integrate with ast_to_c.cpp

/*
#include "ast_to_c.h"
#include "../../ext/mustache/mustache.hpp"
#include <sstream>

using namespace kainjow::mustache;

std::string ASTToCEmitter::emit_with_mustache(const ProgramNode* program) {
    if (!program || program->functions.empty()) {
        return "";
    }
    
    // Function template
    std::string function_template = R"(
int64_t {{name}}({{#parameters}}{{type}} {{name}}{{^last}}, {{/last}}{{/parameters}}) {
{{#body}}
    {{statement}}
{{/body}}
{{^has_return}}
    return 0;
{{/has_return}}
}
)";
    
    mustache func_tmpl{function_template};
    data functions_list{data::type::list};
    
    for (const std::unique_ptr<FunctionNode>& func : program->functions) {
        data func_data;
        func_data["name"] = func->name;
        
        // Parameters
        data params_list{data::type::list};
        for (size_t i = 0; i < func->parameters.size(); ++i) {
            data param;
            param["type"] = "int64_t";
            param["name"] = func->parameters[i].first;
            param["last"] = (i == func->parameters.size() - 1);
            params_list << param;
        }
        func_data["parameters"] = params_list;
        
        // Body statements
        data body_list{data::type::list};
        bool has_return = false;
        for (const std::unique_ptr<StatementNode>& stmt : func->body) {
            std::ostringstream stmt_out;
            _emit_statement(stmt.get(), stmt_out);
            body_list << data{"statement", stmt_out.str()};
            
            if (stmt->get_type() == ASTNode::NodeType::ReturnStatement) {
                has_return = true;
            }
        }
        func_data["body"] = body_list;
        func_data["has_return"] = has_return;
        
        // Render function
        std::ostringstream func_out;
        func_tmpl.render(func_data, func_out);
        functions_list << data{"function_code", func_out.str()};
    }
    
    // Program template
    std::string program_template = R"(
#include <stdint.h>
#include <stdbool.h>

{{#forward_declarations}}
{{declaration}}
{{/forward_declarations}}

{{#functions}}
{{function_code}}
{{/functions}}
)";
    
    mustache prog_tmpl{program_template};
    data program_data;
    
    // Forward declarations
    data forward_decls{data::type::list};
    for (const std::unique_ptr<FunctionNode>& func : program->functions) {
        std::ostringstream decl;
        decl << "int64_t " << func->name << "(";
        for (size_t i = 0; i < func->parameters.size(); ++i) {
            if (i > 0) decl << ", ";
            decl << "int64_t " << func->parameters[i].first;
        }
        decl << ");";
        forward_decls << data{"declaration", decl.str()};
    }
    program_data["forward_declarations"] = forward_decls;
    program_data["functions"] = functions_list;
    
    // Render final program
    std::ostringstream result;
    prog_tmpl.render(program_data, result);
    return result.str();
}
*/

