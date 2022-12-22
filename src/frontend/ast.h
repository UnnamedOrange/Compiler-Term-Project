/**
 * @file ast.h
 * @author UnnamedOrange
 * @brief Define AST types.
 *
 * @copyright Copyright (c) UnnamedOrange. Licensed under the MIT License.
 * See the LICENSE file in the repository root for full license text.
 */

#pragma once

#include <cassert>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <fmt/core.h>

#include "symbol_table.h"
#include "type_system.h"

namespace compiler::ast
{
    inline int global_result_id;
    inline int new_result_id() { return ++global_result_id; }
    inline int global_sequential_id;
    inline std::string new_sequential_id()
    {
        return fmt::format("seq_{}", ++global_sequential_id);
    }
    inline int global_if_id;
    inline std::string new_if_id()
    {
        return fmt::format("if_{}", ++global_if_id);
    }
    inline std::string get_else_id()
    {
        return fmt::format("else_{}", global_if_id);
    }
    inline int global_land_id;
    inline std::string new_land_id()
    {
        return fmt::format("land_{}", ++global_land_id);
    }
    inline std::string get_land_sc_id()
    {
        return fmt::format("land_sc_{}", global_land_id);
    }
    inline int global_lor_id;
    inline std::string new_lor_id()
    {
        return fmt::format("lor_{}", ++global_lor_id);
    }
    inline std::string get_lor_sc_id()
    {
        return fmt::format("lor_sc_{}", global_lor_id);
    }
    inline int global_while_id;
    inline std::string new_while_id()
    {
        return fmt::format("while_{}", ++global_while_id);
    }
    inline std::string get_while_body_id()
    {
        return fmt::format("while_body_{}", global_while_id);
    }
    inline symbol_table_t st;

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
        mutable std::string break_target;
        mutable std::string continue_target;
        void push_down(const ast_t& down) const
        {
            down->break_target = break_target;
            down->continue_target = continue_target;
        }

    public:
        virtual ~ast_base_t() = default;

    public:
        void assign_result_id() const { result_id = new_result_id(); }
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
         * @brief `inline_number` 表示编译期可计算出的常量。
         * 如果一个表达式可以在编译时计算出一个常量，则结果不是 std::nullopt;
         */
        virtual std::optional<int> get_inline_number() const
        {
            return std::nullopt;
        }

    public:
        virtual std::string to_koopa() const { return ""; }
    };

    /**
     * @brief AST of a complete program.
     * CompUnit ::= [CompUnit] (Decl | FuncDef);
     * Fixed:
     * CompUnit ::= DeclOrFuncList;
     */
    class ast_program_t : public ast_base_t
    {
    public:
        std::vector<ast_t> declaration_or_function_items;

    public:
        std::string to_koopa() const override
        {
            std::string ret;

            // 将库函数加入到符号表。
            {
                std::array lib_functions{
                    symbol_function_t{{"getint"}, true},
                    symbol_function_t{{"getch"}, true},
                    symbol_function_t{{"getarray"}, true},
                    symbol_function_t{{"putint"}, false},
                    symbol_function_t{{"putch"}, false},
                    symbol_function_t{{"putarray"}, false},
                    symbol_function_t{{"starttime"}, false},
                    symbol_function_t{{"stoptime"}, false},
                };
                for (size_t i = 0; i < lib_functions.size(); i++)
                {
                    const auto& symbol = lib_functions[i];
                    st.insert(symbol.internal_name, symbol);
                }
            }

            // 输出库函数的声明。
            ret += R"(decl @getint(): i32
decl @getch(): i32
decl @getarray(*i32): i32
decl @putint(i32)
decl @putch(i32)
decl @putarray(i32, *i32)
decl @starttime()
decl @stoptime()

)";

            for (const auto& item : declaration_or_function_items)
                ret += item->to_koopa();
            return ret;
        }
    };

    /**
     * @brief AST of a declaration or function list.
     * DeclOrFuncList ::= (Decl | FuncDef);
     * DeclOrFuncList ::= (Decl | FuncDef) DeclOrFuncList;
     *
     * @note This is a temporary type only used in syntax analysis.
     */
    class ast_declaration_or_function_list_t : public ast_base_t
    {
    public:
        ast_t item;
        std::shared_ptr<ast_declaration_or_function_list_t>
            declaration_or_function_list;
    };

    /**
     * @brief AST of a function.
     * FuncDef ::= FuncType IDENT "(" [FuncFParams] ")" Block;
     * Fixed:
     * FuncDef ::= FuncType IDENT "(" ")" Block;
     * FuncDef ::= FuncType IDENT "(" FuncFParamList ")" Block;
     */
    class ast_function_t : public ast_base_t
    {
    public:
        ast_t function_type;
        std::string function_name;
        std::vector<ast_t> parameters;
        ast_t block;

    public:
        std::string to_koopa() const override;
    };

    /**
     * @brief AST of a parameter list.
     * FuncFParamList ::= FuncFParam;
     * FuncFParamList ::= FuncFParam "," FuncFParamList;
     *
     * @note This is a temporary type only used in syntax analysis.
     */
    class ast_parameter_list_t : public ast_base_t
    {
    public:
        ast_t parameter;
        std::shared_ptr<ast_parameter_list_t> parameter_list;
    };

    /**
     * @brief AST of a parameter.
     * FuncFParam ::= BType IDENT;
     */
    class ast_parameter_t : public ast_base_t
    {
    public:
        ast_t type;
        std::string raw_name;
    };

    /**
     * @brief AST of a block.
     * Block ::= "{" {BlockItem} "}";
     * Fixed:
     * Block ::= "{" "}";
     * Block ::= "{" BlockItemList "}";
     */
    class ast_block_t : public ast_base_t
    {
    public:
        std::vector<ast_t> block_items;

    public:
        std::string to_koopa() const override
        {
            std::string ret;
            st.push();
            for (const auto& item : block_items)
            {
                push_down(item);
                ret += item->to_koopa();
            }
            st.pop();
            return ret;
        }
    };

    /**
     * @brief AST of a block item list.
     * BlockItemList ::= BlockItem;
     * BlockItemList ::= BlockItem BlockItemList;
     *
     * @note This is a temporary type only used in syntax analysis.
     */
    class ast_block_item_list_t : public ast_base_t
    {
    public:
        ast_t block_item;
        std::shared_ptr<ast_block_item_list_t> block_item_list;
    };

    /**
     * @brief AST of a block item;
     * BlockItem ::= Decl | Stmt;
     */
    class ast_block_item_t : public ast_base_t
    {
    public:
        ast_t item; // Declaration or statement.

    public:
        std::string to_koopa() const override
        {
            push_down(item);
            return item->to_koopa();
        }
    };

    /**
     * @brief AST of a statement.
     * Stmt ::= "return" Exp ";";
     */
    class ast_statement_1_t : public ast_base_t
    {
    public:
        ast_t expression;

    public:
        std::string to_koopa() const override
        {
            std::string ret;
            if (auto const_value = expression->get_inline_number())
                ret += fmt::format("    ret {}\n", *const_value);
            else
            {
                ret += expression->to_koopa();
                ret +=
                    fmt::format("    ret %{}\n", expression->get_result_id());
            }
            ret += fmt::format("%{}:\n", new_sequential_id());
            return ret;
        }
    };

    /**
     * @brief AST of a statement.
     * Stmt ::= LVal "=" Exp ";";
     */
    class ast_statement_2_t : public ast_base_t
    {
    public:
        ast_t lvalue;
        ast_t expression;

    public:
        std::string to_koopa() const override;
    };

    /**
     * @brief AST of a statement.
     * Stmt ::= [Exp] ";";
     */
    class ast_statement_3_t : public ast_base_t
    {
    public:
        ast_t expression;

    public:
        std::string to_koopa() const override
        {
            if (expression)
                return expression->to_koopa();
            return "";
        }
    };

    /**
     * @brief AST of a statement.
     * Stmt ::= Block;
     */
    class ast_statement_4_t : public ast_base_t
    {
    public:
        ast_t block;

    public:
        std::string to_koopa() const override
        {
            push_down(block);
            return block->to_koopa();
        }
    };

    /**
     * @brief AST of a statement.
     * Stmt ::= "if" "(" Exp ")" Stmt ["else" Stmt];
     *
     * @note To avoid ambiguity, the syntax is modified in YACC.
     */
    class ast_statement_5_t : public ast_base_t
    {
    public:
        ast_t condition_expression;
        ast_t if_branch;
        ast_t else_branch;

    public:
        std::string to_koopa() const override
        {
            std::string ret;

            std::string if_id = new_if_id();
            std::string else_id = get_else_id();
            std::string next = new_sequential_id();

            push_down(if_branch);
            if (else_branch)
                push_down(else_branch);

            std::string condition_result;
            if (auto const_value = condition_expression->get_inline_number())
            {
                condition_result = std::to_string(*const_value);
            }
            else
            {
                ret += condition_expression->to_koopa();
                condition_result =
                    fmt::format("%{}", condition_expression->get_result_id());
            }
            ret += fmt::format("    br {}, %{}, %{}\n", condition_result, if_id,
                               else_branch ? else_id : next);
            ret += fmt::format("%{}:\n", if_id);
            ret += if_branch->to_koopa();
            ret += fmt::format("    jump %{}\n", next);
            if (else_branch)
            {
                ret += fmt::format("%{}:\n", else_id);
                ret += else_branch->to_koopa();
                ret += fmt::format("    jump %{}\n", next);
            }
            ret += fmt::format("%{}:\n", next);
            return ret;
        }
    };

    /**
     * @brief AST of a statement.
     * Stmt ::= "while" "(" Exp ")" Stmt;
     */
    class ast_statement_6_t : public ast_base_t
    {
    public:
        ast_t condition_expression;
        ast_t while_branch;

    public:
        std::string to_koopa() const override
        {
            std::string ret;

            std::string while_id = new_while_id();
            std::string while_body_id = get_while_body_id();
            std::string next = new_sequential_id();

            while_branch->break_target = next;
            while_branch->continue_target = while_id;

            ret += fmt::format("    jump %{}\n", while_id);

            ret += fmt::format("%{}:\n", while_id);
            {
                std::string condition_result;
                if (auto const_value =
                        condition_expression->get_inline_number())
                {
                    condition_result = std::to_string(*const_value);
                }
                else
                {
                    ret += condition_expression->to_koopa();
                    condition_result = fmt::format(
                        "%{}", condition_expression->get_result_id());
                }

                ret += fmt::format("    br {}, %{}, %{}\n", condition_result,
                                   while_body_id, next);
            }

            ret += fmt::format("%{}:\n", while_body_id);
            {
                ret += while_branch->to_koopa();
                ret += fmt::format("    jump %{}\n", while_id);
            }

            ret += fmt::format("%{}:\n", next);

            return ret;
        }
    };

    /**
     * @brief AST of a statement.
     * Stmt ::= "break" ";"
     */
    class ast_statement_7_t : public ast_base_t
    {
    public:
        std::string to_koopa() const override
        {
            std::string ret;
            ret += fmt::format("    jump %{}\n", break_target);
            ret += fmt::format("%{}:\n", new_sequential_id());
            return ret;
        }
    };

    /**
     * @brief AST of a statement.
     * Stmt ::= "continue" ";"
     */
    class ast_statement_8_t : public ast_base_t
    {
    public:
        std::string to_koopa() const override
        {
            std::string ret;
            ret += fmt::format("    jump %{}\n", continue_target);
            ret += fmt::format("%{}:\n", new_sequential_id());
            return ret;
        }
    };

    /**
     * @brief AST of an expression.
     * Exp ::= LOrExp;
     */
    class ast_expression_t : public ast_base_t
    {
    public:
        ast_t lor_expression;

    public:
        std::optional<int> get_inline_number() const override
        {
            return lor_expression->get_inline_number();
        }

    public:
        std::string to_koopa() const override
        {
            auto ret = lor_expression->to_koopa();
            assign_result_id(lor_expression->get_result_id());
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
        std::optional<int> get_inline_number() const override
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
        std::optional<int> get_inline_number() const override { return number; }
    };

    /**
     * @brief AST of a primary_expression.
     * PrimaryExp ::= LVal;
     */
    class ast_primary_expression_3_t : public ast_base_t
    {
    public:
        ast_t lvalue;

    public:
        std::optional<int> get_inline_number() const override
        {
            return lvalue->get_inline_number();
        }

    public:
        std::string to_koopa() const override
        {
            auto ret = lvalue->to_koopa();
            assign_result_id(lvalue->get_result_id());
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
        std::optional<int> get_inline_number() const override
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
        std::optional<int> get_inline_number() const override
        {
            auto rvalue = unary_expression->get_inline_number();
            if (!rvalue)
                return std::nullopt;

            else if (op == "+")
                return (*rvalue);
            else if (op == "-")
                return -(*rvalue);
            else if (op == "!")
                return !(*rvalue);

            return std::nullopt;
        }

    public:
        std::string to_koopa() const override
        {
            std::string ret;

            std::string operator_name;
            std::string operand[2];

            operand[0] = "0";

            if (auto const_value = unary_expression->get_inline_number())
                operand[1] = std::to_string(*const_value);
            else
            {
                ret += unary_expression->to_koopa();
                operand[1] =
                    fmt::format("%{}", unary_expression->get_result_id());
            }

            if (false)
                ;
            else if (op == "+")
                operator_name = "add";
            else if (op == "-")
                operator_name = "sub";
            else if (op == "!")
                operator_name = "eq";

            assign_result_id();
            ret += fmt::format("    %{} = {} {}, {}\n", get_result_id(),
                               operator_name, operand[0], operand[1]);
            return ret;
        }
    };

    /**
     * @brief AST of a unary expression.
     * UnaryExp ::= IDENT "(" [FuncRParams] ")";
     * Fixed:
     * UnaryExp ::= IDENT "(" ")";
     * UnaryExp ::= IDENT "(" FuncRParamList ")";
     */
    class ast_unary_expression_3_t : public ast_base_t
    {
    public:
        std::string function_raw_name;
        std::vector<ast_t> arguments; // An argument is an expression.

    public:
        std::string to_koopa() const override
        {
            std::string ret;

            auto symbol =
                std::get<symbol_function_t>(*st.at(function_raw_name));

            // Generate argument string.
            std::string argument_string;
            {
                for (size_t i = 0; i < arguments.size(); i++)
                {
                    std::string single_argument_string;
                    const auto& argument = arguments[i];
                    if (auto const_value = argument->get_inline_number())
                        single_argument_string = std::to_string(*const_value);
                    else
                    {
                        ret += argument->to_koopa();
                        single_argument_string =
                            fmt::format("%{}", argument->get_result_id());
                    }

                    if (!argument_string.empty())
                        argument_string += ", ";
                    argument_string += single_argument_string;
                }
            }

            // Generate prefix string.
            std::string prefix_string;
            if (symbol.has_return_value)
            {
                assign_result_id();
                prefix_string = fmt::format("%{} = ", get_result_id());
            }

            ret += fmt::format("    {}call @{}({})\n", prefix_string,
                               symbol.internal_name, argument_string);
            return ret;
        }
    };

    /**
     * @brief AST of a argument list.
     * FuncRParamList ::= Exp;
     * FuncRParamList ::= Exp "," FuncRParamList;
     *
     * @note This is a temporary type only used in syntax analysis.
     */
    class ast_argument_list_t : public ast_base_t
    {
    public:
        ast_t argument;
        std::shared_ptr<ast_argument_list_t> argument_list;
    };

    /**
     * @brief AST of an multiply expression.
     * MulExp ::= UnaryExp;
     */
    class ast_multiply_expression_1_t : public ast_base_t
    {
    public:
        ast_t unary_expression;

    public:
        std::optional<int> get_inline_number() const override
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
     * @brief AST of an multiply expression.
     * MulExp ::= MulExp ("*" | "/" | "%") UnaryExp;
     */
    class ast_multiply_expression_2_t : public ast_base_t
    {
    public:
        ast_t multiply_expression;
        std::string op;
        ast_t unary_expression;

    public:
        std::optional<int> get_inline_number() const override
        {
            auto rvalue_1 = multiply_expression->get_inline_number();
            if (!rvalue_1)
                return std::nullopt;
            auto rvalue_2 = unary_expression->get_inline_number();
            if (!rvalue_2)
                return std::nullopt;

            else if (op == "*")
                return (*rvalue_1) * (*rvalue_2);
            else if (op == "/")
                return (*rvalue_1) / (*rvalue_2);
            else if (op == "%")
                return (*rvalue_1) % (*rvalue_2);

            return std::nullopt;
        }

    public:
        std::string to_koopa() const override
        {
            std::string ret;

            std::string operator_name;
            std::string operand[2];

            if (auto const_value = multiply_expression->get_inline_number())
                operand[0] = std::to_string(*const_value);
            else
            {
                ret += multiply_expression->to_koopa();
                operand[0] =
                    fmt::format("%{}", multiply_expression->get_result_id());
            }

            if (auto const_value = unary_expression->get_inline_number())
                operand[1] = std::to_string(*const_value);
            else
            {
                ret += unary_expression->to_koopa();
                operand[1] =
                    fmt::format("%{}", unary_expression->get_result_id());
            }

            if (false)
                ;
            else if (op == "*")
                operator_name = "mul";
            else if (op == "/")
                operator_name = "div";
            else if (op == "%")
                operator_name = "mod";

            assign_result_id();
            ret += fmt::format("    %{} = {} {}, {}\n", get_result_id(),
                               operator_name, operand[0], operand[1]);
            return ret;
        }
    };

    /**
     * @brief AST of an add expression.
     * AddExp ::= MulExp;
     */
    class ast_add_expression_1_t : public ast_base_t
    {
    public:
        ast_t multiply_expression;

    public:
        std::optional<int> get_inline_number() const override
        {
            return multiply_expression->get_inline_number();
        }

    public:
        std::string to_koopa() const override
        {
            auto ret = multiply_expression->to_koopa();
            assign_result_id(multiply_expression->get_result_id());
            return ret;
        }
    };

    /**
     * @brief AST of an add expression.
     * AddExp ::= AddExp ("+" | "-") MulExp;
     */
    class ast_add_expression_2_t : public ast_base_t
    {
    public:
        ast_t add_expression;
        std::string op;
        ast_t multiply_expression;

    public:
        std::optional<int> get_inline_number() const override
        {
            auto rvalue_1 = add_expression->get_inline_number();
            if (!rvalue_1)
                return std::nullopt;
            auto rvalue_2 = multiply_expression->get_inline_number();
            if (!rvalue_2)
                return std::nullopt;

            else if (op == "+")
                return (*rvalue_1) + (*rvalue_2);
            else if (op == "-")
                return (*rvalue_1) - (*rvalue_2);

            return std::nullopt;
        }

    public:
        std::string to_koopa() const override
        {
            std::string ret;

            std::string operator_name;
            std::string operand[2];

            if (auto const_value = add_expression->get_inline_number())
                operand[0] = std::to_string(*const_value);
            else
            {
                ret += add_expression->to_koopa();
                operand[0] =
                    fmt::format("%{}", add_expression->get_result_id());
            }

            if (auto const_value = multiply_expression->get_inline_number())
                operand[1] = std::to_string(*const_value);
            else
            {
                ret += multiply_expression->to_koopa();
                operand[1] =
                    fmt::format("%{}", multiply_expression->get_result_id());
            }

            if (false)
                ;
            else if (op == "+")
                operator_name = "add";
            else if (op == "-")
                operator_name = "sub";

            assign_result_id();
            ret += fmt::format("    %{} = {} {}, {}\n", get_result_id(),
                               operator_name, operand[0], operand[1]);
            return ret;
        }
    };

    /**
     * @brief AST of a relation expression.
     * RelExp ::= AddExp;
     */
    class ast_relation_expression_1_t : public ast_base_t
    {
    public:
        ast_t add_expression;

    public:
        std::optional<int> get_inline_number() const override
        {
            return add_expression->get_inline_number();
        }

    public:
        std::string to_koopa() const override
        {
            auto ret = add_expression->to_koopa();
            assign_result_id(add_expression->get_result_id());
            return ret;
        }
    };

    /**
     * @brief AST of a relation expression.
     * RelExp ::= RelExp ("<" | ">" | "<=" | ">=") AddExp;
     */
    class ast_relation_expression_2_t : public ast_base_t
    {
    public:
        ast_t relation_expression;
        std::string op;
        ast_t add_expression;

    public:
        std::optional<int> get_inline_number() const override
        {
            auto rvalue_1 = relation_expression->get_inline_number();
            if (!rvalue_1)
                return std::nullopt;
            auto rvalue_2 = add_expression->get_inline_number();
            if (!rvalue_2)
                return std::nullopt;

            else if (op == "<")
                return (*rvalue_1) < (*rvalue_2);
            else if (op == ">")
                return (*rvalue_1) > (*rvalue_2);
            else if (op == "<=")
                return (*rvalue_1) <= (*rvalue_2);
            else if (op == ">=")
                return (*rvalue_1) >= (*rvalue_2);

            return std::nullopt;
        }

    public:
        std::string to_koopa() const override
        {
            std::string ret;

            std::string operator_name;
            std::string operand[2];

            if (auto const_value = relation_expression->get_inline_number())
                operand[0] = std::to_string(*const_value);
            else
            {
                ret += relation_expression->to_koopa();
                operand[0] =
                    fmt::format("%{}", relation_expression->get_result_id());
            }

            if (auto const_value = add_expression->get_inline_number())
                operand[1] = std::to_string(*const_value);
            else
            {
                ret += add_expression->to_koopa();
                operand[1] =
                    fmt::format("%{}", add_expression->get_result_id());
            }

            if (false)
                ;
            else if (op == "<")
                operator_name = "lt";
            else if (op == ">")
                operator_name = "gt";
            else if (op == "<=")
                operator_name = "le";
            else if (op == ">=")
                operator_name = "ge";

            assign_result_id();
            ret += fmt::format("    %{} = {} {}, {}\n", get_result_id(),
                               operator_name, operand[0], operand[1]);
            return ret;
        }
    };

    /**
     * @brief AST of an equation expression.
     * EqExp ::= RelExp;
     */
    class ast_equation_expression_1_t : public ast_base_t
    {
    public:
        ast_t relation_expression;

    public:
        std::optional<int> get_inline_number() const override
        {
            return relation_expression->get_inline_number();
        }

    public:
        std::string to_koopa() const override
        {
            auto ret = relation_expression->to_koopa();
            assign_result_id(relation_expression->get_result_id());
            return ret;
        }
    };

    /**
     * @brief AST of an equation expression.
     * EqExp ::= EqExp ("==" | "!=") RelExp;
     */
    class ast_equation_expression_2_t : public ast_base_t
    {
    public:
        ast_t equation_expression;
        std::string op;
        ast_t relation_expression;

    public:
        std::optional<int> get_inline_number() const override
        {
            auto rvalue_1 = equation_expression->get_inline_number();
            if (!rvalue_1)
                return std::nullopt;
            auto rvalue_2 = relation_expression->get_inline_number();
            if (!rvalue_2)
                return std::nullopt;

            else if (op == "==")
                return (*rvalue_1) == (*rvalue_2);
            else if (op == "!=")
                return (*rvalue_1) != (*rvalue_2);

            return std::nullopt;
        }

    public:
        std::string to_koopa() const override
        {
            std::string ret;

            std::string operator_name;
            std::string operand[2];

            if (auto const_value = equation_expression->get_inline_number())
                operand[0] = std::to_string(*const_value);
            else
            {
                ret += equation_expression->to_koopa();
                operand[0] =
                    fmt::format("%{}", equation_expression->get_result_id());
            }

            if (auto const_value = relation_expression->get_inline_number())
                operand[1] = std::to_string(*const_value);
            else
            {
                ret += relation_expression->to_koopa();
                operand[1] =
                    fmt::format("%{}", relation_expression->get_result_id());
            }

            if (false)
                ;
            else if (op == "==")
                operator_name = "eq";
            else if (op == "!=")
                operator_name = "ne";

            assign_result_id();
            ret += fmt::format("    %{} = {} {}, {}\n", get_result_id(),
                               operator_name, operand[0], operand[1]);
            return ret;
        }
    };

    /**
     * @brief AST of an land expression.
     * LAndExp ::= EqExp;
     */
    class ast_land_expression_1_t : public ast_base_t
    {
    public:
        ast_t equation_expression;

    public:
        std::optional<int> get_inline_number() const override
        {
            return equation_expression->get_inline_number();
        }

    public:
        std::string to_koopa() const override
        {
            auto ret = equation_expression->to_koopa();
            assign_result_id(equation_expression->get_result_id());
            return ret;
        }
    };

    /**
     * @brief AST of an land expression.
     * LAndExp ::= LAndExp "&&" EqExp;
     */
    class ast_land_expression_2_t : public ast_base_t
    {
    public:
        ast_t land_expression;
        ast_t equation_expression;

    public:
        std::optional<int> get_inline_number() const override
        {
            auto rvalue_1 = land_expression->get_inline_number();
            if (!rvalue_1)
                return std::nullopt;
            if (!(*rvalue_1))
                return 0; // Short circuit.
            auto rvalue_2 = equation_expression->get_inline_number();
            if (!rvalue_2)
                return std::nullopt;

            return (*rvalue_1) && (*rvalue_2);
        }

    public:
        std::string to_koopa() const override
        {
            std::string ret;

            std::string operand[2];
            std::string true_branch = new_land_id();
            std::string false_branch = get_land_sc_id();
            std::string next = new_sequential_id();
            int temp_result_id = new_result_id();

            ret += fmt::format("    %{} = alloc i32\n", temp_result_id);
            ret += fmt::format("    store 1, %{}\n", temp_result_id);

            if (auto const_value = land_expression->get_inline_number())
                operand[0] = std::to_string(*const_value);
            else
            {
                ret += land_expression->to_koopa();
                operand[0] =
                    fmt::format("%{}", land_expression->get_result_id());
            }

            // Short circuit.
            ret += fmt::format("    br {}, %{}, %{}\n", operand[0], true_branch,
                               false_branch);

            ret += fmt::format("%{}:\n", true_branch);
            {
                if (auto const_value = equation_expression->get_inline_number())
                    operand[1] = std::to_string(*const_value);
                else
                {
                    ret += equation_expression->to_koopa();
                    operand[1] = fmt::format(
                        "%{}", equation_expression->get_result_id());
                }

                int bool_value[2];
                for (size_t i = 0; i < 2; i++)
                {
                    bool_value[i] = new_result_id();
                    ret += fmt::format("    %{} = ne {}, 0\n", bool_value[i],
                                       operand[i]);
                }

                int bool_result = new_result_id();
                ret += fmt::format("    %{} = and %{}, %{}\n", bool_result,
                                   bool_value[0], bool_value[1]);
                ret += fmt::format("    store %{}, %{}\n", bool_result,
                                   temp_result_id);

                ret += fmt::format("    jump %{}\n", next);
            }

            ret += fmt::format("%{}:\n", false_branch);
            {
                ret += fmt::format("    store 0, %{}\n", temp_result_id);
                ret += fmt::format("    jump %{}\n", next);
            }

            ret += fmt::format("%{}:\n", next);
            assign_result_id();
            ret += fmt::format("    %{} = load %{}\n", get_result_id(),
                               temp_result_id);

            return ret;
        }
    };

    /**
     * @brief AST of an lor expression.
     * LOrExp ::= LAndExp;
     */
    class ast_lor_expression_1_t : public ast_base_t
    {
    public:
        ast_t land_expression;

    public:
        std::optional<int> get_inline_number() const override
        {
            return land_expression->get_inline_number();
        }

    public:
        std::string to_koopa() const override
        {
            auto ret = land_expression->to_koopa();
            assign_result_id(land_expression->get_result_id());
            return ret;
        }
    };

    /**
     * @brief AST of an lor expression.
     * LOrExp ::= LOrExp "||" LAndExp;
     */
    class ast_lor_expression_2_t : public ast_base_t
    {
    public:
        ast_t lor_expression;
        ast_t land_expression;

    public:
        std::optional<int> get_inline_number() const override
        {
            auto rvalue_1 = lor_expression->get_inline_number();
            if (!rvalue_1)
                return std::nullopt;
            if (*rvalue_1)
                return 1; // Short circuit.
            auto rvalue_2 = land_expression->get_inline_number();
            if (!rvalue_2)
                return std::nullopt;

            return (*rvalue_1) || (*rvalue_2);
        }

    public:
        std::string to_koopa() const override
        {
            std::string ret;

            assign_result_id();

            std::string operand[2];
            std::string false_branch = new_lor_id();
            std::string true_branch = get_lor_sc_id();
            std::string next = new_sequential_id();
            int temp_result_id = new_result_id();

            ret += fmt::format("    %{} = alloc i32\n", temp_result_id);
            ret += fmt::format("    store 0, %{}\n", temp_result_id);

            if (auto const_value = lor_expression->get_inline_number())
                operand[0] = std::to_string(*const_value);
            else
            {
                ret += lor_expression->to_koopa();
                operand[0] =
                    fmt::format("%{}", lor_expression->get_result_id());
            }

            // Short circuit.
            ret += fmt::format("    br {}, %{}, %{}\n", operand[0], true_branch,
                               false_branch);

            ret += fmt::format("%{}:\n", false_branch);
            {
                if (auto const_value = land_expression->get_inline_number())
                    operand[1] = std::to_string(*const_value);
                else
                {
                    ret += land_expression->to_koopa();
                    operand[1] =
                        fmt::format("%{}", land_expression->get_result_id());
                }

                int bool_value[2];
                for (size_t i = 0; i < 2; i++)
                {
                    bool_value[i] = new_result_id();
                    ret += fmt::format("    %{} = ne {}, 0\n", bool_value[i],
                                       operand[i]);
                }

                int bool_result = new_result_id();
                ret += fmt::format("    %{} = or %{}, %{}\n", bool_result,
                                   bool_value[0], bool_value[1]);
                ret += fmt::format("    store %{}, %{}\n", bool_result,
                                   temp_result_id);

                ret += fmt::format("    jump %{}\n", next);
            }

            ret += fmt::format("%{}:\n", true_branch);
            {
                ret += fmt::format("    store 1, %{}\n", temp_result_id);
                ret += fmt::format("    jump %{}\n", next);
            }

            ret += fmt::format("%{}:\n", next);
            assign_result_id();
            ret += fmt::format("    %{} = load %{}\n", get_result_id(),
                               temp_result_id);

            return ret;
        }
    };

    /**
     * @brief AST of a declaration statement.
     * Decl ::= ConstDecl;
     */
    class ast_declaration_1_t : public ast_base_t
    {
    public:
        ast_t const_declaration;

    public:
        std::string to_koopa() const override
        {
            return const_declaration->to_koopa();
        }
    };

    /**
     * @brief AST of a declaration statement.
     * Decl ::= VarDecl;
     */
    class ast_declaration_2_t : public ast_base_t
    {
    public:
        ast_t variable_declaration;

    public:
        std::string to_koopa() const override
        {
            return variable_declaration->to_koopa();
        }
    };

    /**
     * @brief AST of a type.
     * Type ::= "void" | "int";
     */
    class ast_type_t : public ast_base_t
    {
    public:
        std::shared_ptr<type_base_t> type;

    public:
        std::string to_koopa() const override { return type->to_koopa(); }
    };

    /**
     * @brief AST of a const declaration statement.
     * ConstDecl ::= "const" BType ConstDef {"," ConstDef} ";";
     * Fixed:
     * ConstDecl ::= "const" BType ConstDefList ";";
     */
    class ast_const_declaration_t : public ast_base_t
    {
    public:
        ast_t primary_type;
        std::vector<ast_t> const_definitions;

    public:
        std::string to_koopa() const override
        {
            for (const auto& def : const_definitions)
                def->to_koopa();
            return "";
        }
    };

    /**
     * @brief AST of a const definition list.
     * ConstDefList ::= ConstDef;
     * ConstDefList ::= ConstDef "," ConstDefList;
     *
     * @note This is a temporary type only used in syntax analysis.
     */
    class ast_const_definition_list_t : public ast_base_t
    {
    public:
        ast_t const_definition;
        std::shared_ptr<ast_const_definition_list_t> const_definition_list;
    };

    /**
     * @brief AST of a const definition.
     * ConstDef ::= IDENT {"[" ConstExp "]"} "=" ConstInitVal;
     * Fixed:
     * ConstDef ::= IDENT "=" ConstInitVal;
     * ConstDef ::= IDENT ArrDimList "=" ConstInitVal;
     */
    class ast_const_definition_t : public ast_base_t
    {
    public:
        std::shared_ptr<ast_type_t> type;
        std::string raw_name;
        ast_t const_initial_value;

    public:
        std::string to_koopa() const override
        {
            symbol_const_t symbol;
            symbol.value = *const_initial_value->get_inline_number();
            st.insert(raw_name, std::move(symbol));
            // TODO: Implement.
            return "";
        }
    };

    /**
     * @brief AST of an array dimension list.
     * ArrDimList ::= ArrDim;
     * ArrDimList ::= ArrDim ArrDimList;
     *
     * @note This is a temporary type only used in syntax analysis.
     */
    class ast_array_dimension_list_t : public ast_base_t
    {
    public:
        ast_t array_dimension; // An array dimension is a const expression.
        std::shared_ptr<ast_array_dimension_list_t> array_dimension_list;
    };

    /**
     * @brief AST of a const initial value list.
     * ConstInitValList ::= ConstInitVal;
     * ConstInitValList ::= ConstInitVal "," ConstInitValList;
     *
     * @note This is a temporary type only used in syntax analysis.
     */
    class ast_const_initial_value_list_t : public ast_base_t
    {
    public:
        ast_t const_initial_value;
        std::shared_ptr<ast_const_initial_value_list_t>
            const_initial_value_list;
    };

    /**
     * @brief AST of a const initial value.
     * ConstInitVal ::= ConstExp;
     */
    class ast_const_initial_value_1_t : public ast_base_t
    {
    public:
        ast_t const_expression;

    public:
        std::optional<int> get_inline_number() const override
        {
            return const_expression->get_inline_number();
        }

    public:
        std::string to_koopa() const override
        {
            return const_expression->to_koopa();
        }
    };

    /**
     * @brief AST of a const initial value.
     * ConstInitVal ::= "{" [ConstInitVal {"," ConstInitVal}] "}";
     * Fixed:
     * ConstInitVal ::= "{" "}";
     * ConstInitVal ::= "{" ConstInitValList "}";
     */
    class ast_const_initial_value_2_t : public ast_base_t
    {
    public:
        std::vector<std::shared_ptr<ast_base_t>> const_initial_values;
    };

    /**
     * @brief AST of a const expression.
     * ConstExp ::= Exp;
     */
    class ast_const_expression_t : public ast_base_t
    {
    public:
        ast_t expression;

    public:
        std::optional<int> get_inline_number() const override
        {
            return expression->get_inline_number();
        }

    public:
        std::string to_koopa() const override { return expression->to_koopa(); }
    };

    /**
     * @brief AST of a variable declaration statement.
     * VarDecl ::= BType VarDef {"," VarDef} ";";
     * Fixed:
     * VarDecl ::= BType VarDefList ";";
     */
    class ast_variable_declaration_t : public ast_base_t
    {
    public:
        ast_t primary_type;
        std::vector<ast_t> variable_definitions;

    public:
        std::string to_koopa() const override
        {
            std::string ret;
            for (const auto& def : variable_definitions)
                ret += def->to_koopa();
            return ret;
        }
    };

    /**
     * @brief AST of a variable definition list.
     * VarDefList ::= VarDef;
     * VarDefList ::= VarDef "," VarDefList;
     *
     * @note This is a temporary type only used in syntax analysis.
     */
    class ast_variable_definition_list_t : public ast_base_t
    {
    public:
        ast_t variable_definition;
        std::shared_ptr<ast_variable_definition_list_t>
            variable_definition_list;
    };

    /**
     * @brief AST of a variable definition.
     * Base class holding the type and name.
     */
    class ast_variable_definition_t : public ast_base_t
    {
    public:
        std::shared_ptr<ast_type_t> type;
        std::string raw_name;

    public:
        std::string to_koopa() const override
        {
            std::string ret;

            {
                symbol_variable_t symbol;
                st.insert(raw_name, std::move(symbol));
            }

            auto symbol = std::get<symbol_variable_t>(*st.at(raw_name));
            auto type_string = type->to_koopa();

            if (st.is_global(raw_name))
            {
                ret += fmt::format(
                    "global @{} = alloc {}, ", // This line does not end.
                    symbol.internal_name, type_string);
            }
            else
            {
                ret += fmt::format("    @{} = alloc {}\n", symbol.internal_name,
                                   type_string);
            }

            return ret;
        }
    };

    /**
     * @brief AST of a variable definition.
     * VarDef ::= IDENT {"[" ConstExp "]"};
     * Fixed:
     * VarDef ::= IDENT;
     * VarDef ::= IDENT ArrDimList;
     */
    class ast_variable_definition_1_t : public ast_variable_definition_t
    {
    public:
        std::string to_koopa() const override
        {
            // TODO: Implement.
            auto ret = ast_variable_definition_t::to_koopa();
            if (st.is_global(raw_name))
                ret += "zeroinit\n\n";
            return ret;
        }
    };

    /**
     * @brief AST of a variable definition.
     * VarDef ::= IDENT {"[" ConstExp "]"} "=" InitVal;
     * Fixed:
     * VarDef ::= IDENT "=" InitVal;
     * VarDef ::= IDENT ArrDimList "=" InitVal;
     */
    class ast_variable_definition_2_t : public ast_variable_definition_t
    {
    public:
        ast_t initial_value;

    public:
        std::string to_koopa() const override
        {
            // TODO: Implement.
            auto ret = ast_variable_definition_t::to_koopa();

            std::string initial_value_holder;
            auto const_initial_value = initial_value->get_inline_number();
            // 判断初始化表达式是否是常量值。
            {
                if (const_initial_value)
                    initial_value_holder = std::to_string(*const_initial_value);
                else
                {
                    ret += initial_value->to_koopa();
                    initial_value_holder =
                        fmt::format("%{}", initial_value->get_result_id());
                }
            }

            if (st.is_global(raw_name))
            {
                // 只允许使用常量表达式初始化全局变量。
                assert(const_initial_value);
                ret += fmt::format("{}\n\n", *const_initial_value);
            }
            else
            {
                auto symbol = std::get<symbol_variable_t>(*st.at(raw_name));
                ret += fmt::format("    store {}, @{}\n", initial_value_holder,
                                   symbol.internal_name);
            }

            return ret;
        }
    };

    /**
     * @brief AST of an initial value list.
     * InitValList ::= InitVal;
     * InitValList ::= InitVal "," InitValList;
     *
     * @note This is a temporary type only used in syntax analysis.
     */
    class ast_initial_value_list_t : public ast_base_t
    {
    public:
        ast_t initial_value;
        std::shared_ptr<ast_initial_value_list_t> initial_value_list;
    };

    /**
     * @brief AST of an initial value.
     * InitVal ::= Exp;
     */
    class ast_initial_value_1_t : public ast_base_t
    {
    public:
        ast_t expression;

    public:
        std::optional<int> get_inline_number() const override
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
     * @brief AST of a const initial value.
     * InitVal ::= "{" [InitVal {"," InitVal}] "}";
     * Fixed:
     * InitVal ::= "{" "}";
     * InitVal ::= "{" InitValList "}";
     */
    class ast_initial_value_2_t : public ast_base_t
    {
    public:
        std::vector<std::shared_ptr<ast_base_t>> initial_values;
    };

    /**
     * @brief AST of an lvalue.
     * LVal ::= IDENT {"[" Exp "]"};
     * Fixed:
     * LVal ::= IDENT;
     * LVal ::= IDENT IdxList;
     */
    class ast_lvalue_t : public ast_base_t
    {
    public:
        std::string raw_name;
        std::vector<ast_t> indices; // An index is an expression.

    public:
        std::optional<int> get_inline_number() const override
        {
            auto symbol = st.at(raw_name);
            if (!symbol)
                return std::nullopt;
            if (!std::holds_alternative<symbol_const_t>(*symbol))
                return std::nullopt;

            return std::get<symbol_const_t>(*symbol).value;
        }

    public:
        std::string to_koopa() const override
        {
            // TODO: Implement.
            assign_result_id(); // Always assign a new id to load.
            auto symbol = std::get<symbol_variable_t>(*st.at(raw_name));
            return fmt::format("    %{} = load @{}\n", get_result_id(),
                               symbol.internal_name);
        }
    };

    /**
     * @brief AST of an index list.
     * IdxList ::= Idx;
     * IdxList ::= Idx IdxList;
     *
     * @note This is a temporary type only used in syntax analysis.
     */
    class ast_index_list_t : public ast_base_t
    {
    public:
        ast_t index;
        std::shared_ptr<ast_index_list_t> index_list;
    };

    inline std::string ast_function_t::to_koopa() const
    {
        std::string ret;

        // Insert the function into symbol table.
        {
            symbol_function_t symbol;
            symbol.has_return_value = !function_type->to_koopa().empty();
            st.insert(function_name, symbol);
        }

        st.push();

        std::string parameter_string;
        for (size_t i = 0; i < parameters.size(); i++)
        {
            auto param =
                std::dynamic_pointer_cast<ast_parameter_t>(parameters[i]);
            if (!parameter_string.empty())
                parameter_string += ", ";
            parameter_string += fmt::format("@{}: {}", param->raw_name,
                                            param->type->to_koopa());
        }

        std::string return_type_string;
        {
            return_type_string = function_type->to_koopa();
            if (!return_type_string.empty())
                return_type_string = fmt::format(": {}", return_type_string);
        }
        ret += fmt::format("fun @{}({}){} {{\n", function_name,
                           parameter_string, return_type_string);

        ret += fmt::format("%{}_entry:\n", function_name);
        for (size_t i = 0; i < parameters.size(); i++)
        {
            auto param =
                std::dynamic_pointer_cast<ast_parameter_t>(parameters[i]);
            st.insert(param->raw_name, symbol_variable_t{});
            auto symbol = std::get<symbol_variable_t>(*st.at(param->raw_name));

            ret += fmt::format("    @{} = alloc {}\n", symbol.internal_name,
                               param->type->to_koopa());
            ret += fmt::format("    store @{}, @{}\n", param->raw_name,
                               symbol.internal_name);
        }

        ret += block->to_koopa();
        if (function_type->to_koopa().empty())
            ret += "    ret\n";
        else
            ret += "    ret 0\n";
        ret += "}\n\n";

        st.pop();

        return ret;
    }

    inline std::string ast_statement_2_t::to_koopa() const
    {
        std::string ret;

        std::string expression_holder;
        if (auto const_value = expression->get_inline_number())
        {
            expression_holder = std::to_string(*const_value);
        }
        else
        {
            ret += expression->to_koopa();
            expression_holder = fmt::format("%{}", expression->get_result_id());
        }

        // Always regrad lvalue as a name.
        // ret += lvalue->to_koopa();
        {
            auto ast_lvalue = std::dynamic_pointer_cast<ast_lvalue_t>(lvalue);
            auto symbol =
                std::get<symbol_variable_t>(*st.at(ast_lvalue->raw_name));

            ret += fmt::format("    store {}, @{}\n", expression_holder,
                               symbol.internal_name);
        }
        return ret;
    }
} // namespace compiler::ast
