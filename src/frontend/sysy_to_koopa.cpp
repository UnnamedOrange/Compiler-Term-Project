/**
 * @file sysy_to_koopa.cpp
 * @author UnnamedOrange
 * @brief Compile SysY to Koopa IR.
 *
 * @copyright Copyright (c) UnnamedOrange. Licensed under the MIT License.
 * See the LICENSE file in the repository root for full license text.
 */

#include "sysy_to_koopa.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include <fmt/core.h>

#include <ast/ast.h>
#include <parser/yy_interface.h>
#include <utility.hpp>

using namespace compiler;

std::string sysy_to_koopa::compile(const std::filesystem::path& input_file_path)
{
    using namespace ast;
    c_file input_file;
    ast_t ast;

    // Open the input file and assign it to yyin.
    {
        try
        {
            input_file = c_file::open(input_file_path, "r");
        }
        catch (const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
            std::exit(1);
        }
        yyin = input_file;
    }

    // Parse the input file to get AST.
    {
        int result = yyparse(ast);
        if (result)
        {
            std::cerr << fmt::format("[Error] YACC failed with error code {}.",
                                     result)
                      << std::endl;
            std::exit(result);
        }
    }

    return ast->to_koopa();
}
