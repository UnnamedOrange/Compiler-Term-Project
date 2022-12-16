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
    inline int global_result_id;

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
    private:
        mutable int result_id{};

    public:
        virtual ~ast_base_t() = default;

    public:
        void assign_result_id() const { result_id = ++global_result_id; }
        void assign_result_id(int existing_result_id) const
        {
            result_id = existing_result_id;
        }
        /**
         * @brief `result_id` 表示 Koopa 中临时变量的编号。
         * 0 表示某个表达式不对应任何运算结果，可能需要使用内联数。
         */
        int get_result_id() const { return result_id; }

        /**
         * @brief `inline_number` 表示 Koopa 中的整数字面量。
         * 如果一个表达式不应该对应整数字面量，则结果为 0。
         */
        virtual int get_inline_number() const { return 0; }

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
     * Stmt ::= "return" Exp ";";
     */
    class ast_statement_t : public ast_base_t
    {
    public:
        ast_t expression;

    public:
        std::string to_koopa() const override
        {
            auto ret = expression->to_koopa();
            if (int id = expression->get_result_id())
                return ret + fmt::format("ret %{}\n", id);
            else
                return ret +
                       fmt::format("ret {}\n", expression->get_inline_number());
        }
    };

    /**
     * @brief AST of an expression.
     * Exp ::= UnaryExp;
     */
    class ast_expression_t : public ast_base_t
    {
    public:
        ast_t unary_expression;

    public:
        int get_inline_number() const override
        {
            return unary_expression->get_inline_number();
        }

    public:
        std::string to_koopa() const override
        {
            auto ret = unary_expression->to_koopa();
            assign_result_id(unary_expression->get_result_id());
            return ret;
        }
    };

    /**
     * @brief AST of a unary expression.
     * UnaryExp ::= PrimaryExp;
     */
    class ast_unary_expression_1_t : public ast_base_t
    {
    public:
        ast_t primary_expression;

    public:
        int get_inline_number() const override
        {
            return primary_expression->get_inline_number();
        }

    public:
        std::string to_koopa() const override
        {
            auto ret = primary_expression->to_koopa();
            assign_result_id(primary_expression->get_result_id());
            return ret;
        }
    };

    /**
     * @brief AST of a unary expression.
     * UnaryExp ::= UnaryOp UnaryExp;
     */
    class ast_unary_expression_2_t : public ast_base_t
    {
    public:
        std::string op;
        ast_t unary_expression;

    public:
        int get_inline_number() const override
        {
            return unary_expression->get_inline_number();
        }

    public:
        std::string to_koopa() const override
        {
            auto ret = unary_expression->to_koopa();
            if (false)
                ;
            else if (op == "+")
            {
                assign_result_id(unary_expression->get_result_id());
            }
            else if (op == "-")
            {
                assign_result_id();
                if (int id = unary_expression->get_result_id())
                {
                    ret +=
                        fmt::format("%{} = sub 0, %{}\n", get_result_id(), id);
                }
                else
                {
                    ret += fmt::format("%{} = sub 0, {}\n", get_result_id(),
                                       unary_expression->get_inline_number());
                }
            }
            else if (op == "!")
            {
                assign_result_id();
                if (int id = unary_expression->get_result_id())
                {
                    ret +=
                        fmt::format("%{} = eq %{}, 0\n", get_result_id(), id);
                }
                else
                {
                    ret += fmt::format("%{} = eq {}, 0\n", get_result_id(),
                                       unary_expression->get_inline_number());
                }
            }
            return ret;
        }
    };

    /**
     * @brief AST of a primary_expression.
     * PrimaryExp ::= "(" Exp ")";
     */
    class ast_primary_expression_1_t : public ast_base_t
    {
    public:
        ast_t expression;

    public:
        int get_inline_number() const override
        {
            return expression->get_inline_number();
        }

    public:
        std::string to_koopa() const override
        {
            auto ret = expression->to_koopa();
            assign_result_id(expression->get_result_id());
            return ret;
        }
    };

    /**
     * @brief AST of a primary_expression.
     * PrimaryExp ::= Number;
     */
    class ast_primary_expression_2_t : public ast_base_t
    {
    public:
        int number;

    public:
        int get_inline_number() const override { return number; }

    public:
        std::string to_koopa() const override { return ""; }
    };
} // namespace compiler::ast
