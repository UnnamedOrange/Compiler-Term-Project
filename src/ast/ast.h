/**
 * @file ast.h
 * @author UnnamedOrange
 * @brief Define AST types.
 *
 * @copyright Copyright (c) UnnamedOrange. Licensed under the MIT License.
 * See the LICENSE file in the repository root for full license text.
 */

#pragma once

#include <memory>
#include <string>

#include <fmt/core.h>

namespace compiler::ast
{
    class ast_base_t;
    /**
     * @brief General AST type using smart pointer.
     */
    using ast_t = std::shared_ptr<ast_base_t>;

    /**
     * @brief AST base class.
     */
    class ast_base_t
    {
    public:
        virtual ~ast_base_t() = default;

    public:
        virtual std::string to_koopa() const = 0;
    };

    /**
     * @brief AST of a complete program.
     */
    class ast_program_t : public ast_base_t
    {
    public:
        ast_t function;

    public:
        std::string to_koopa() const override { return function->to_koopa(); }
    };

    /**
     * @brief AST of a function.
     */
    class ast_function_t : public ast_base_t
    {
    public:
        ast_t function_type;
        std::string function_name;
        ast_t block;

    public:
        std::string to_koopa() const override
        {
            return fmt::format(R"(fun @{}(): {} {{
{}}}
)",
                               function_name, function_type->to_koopa(),
                               block->to_koopa());
        }
    };

    /**
     * @brief AST of a function type.
     */
    class ast_function_type_t : public ast_base_t
    {
    public:
        std::string type_name;

    public:
        std::string to_koopa() const override
        {
            if (type_name == "int")
                return "i32";
            return type_name;
        }
    };

    /**
     * @brief AST of a block.
     */
    class ast_block_t : public ast_base_t
    {
    public:
        ast_t statement;

    public:
        std::string to_koopa() const override
        {
            return fmt::format("%entry:\n{}", statement->to_koopa());
        }
    };

    /**
     * @brief AST of a statement.
     */
    class ast_statement_t : public ast_base_t
    {
    public:
        int number;

    public:
        std::string to_koopa() const override
        {
            return fmt::format("ret {}\n", number);
        }
    };
} // namespace compiler::ast
