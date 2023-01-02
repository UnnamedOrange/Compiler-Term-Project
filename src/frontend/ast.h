/**
 * @file ast.h
 * @author UnnamedOrange
 * @brief Define AST types.
 *
 * @copyright Copyright (c) UnnamedOrange. Licensed under the MIT License.
 * See the LICENSE file in the repository root for full license text.
 */

#pragma once

#include <algorithm>
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
     * @brief A utility AST type that forms a list structure.
     *
     * @note This is a temporary type only used in syntax analysis.
     */
    class ast_list_t : public ast_base_t,
                       public std::enable_shared_from_this<ast_list_t>
    {
    public:
        ast_t value;
        std::shared_ptr<ast_list_t> next;

    public:
        ast_list_t() = default;
        explicit ast_list_t(ast_t value) : value(value) {}
        ast_list_t(ast_t value, ast_t next)
            : value(value), next(std::dynamic_pointer_cast<ast_list_t>(next))
        {
        }

    public:
        std::vector<ast_t> to_vector() const
        {
            std::vector<ast_t> ret;
            auto current_list = shared_from_this();
            while (current_list)
            {
                ret.push_back(current_list->value);
                current_list = current_list->next;
            }
            return ret;
        }
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

            // 将库函数加入到符号表并输出库函数的声明。
            {
                using f = symbol_function_t;
                std::array lib_functions{
                    f{"getint", int_type()},
                    f{"getch", int_type()},
                    f{"getarray", int_type(*int_type)},
                    f{"putint", void_type(int_type)},
                    f{"putch", void_type(int_type)},
                    f{"putarray", void_type(int_type, *int_type)},
                    f{"starttime", void_type()},
                    f{"stoptime", void_type()},
                };
                for (size_t i = 0; i < lib_functions.size(); i++)
                {
                    const auto& symbol = lib_functions[i];
                    st.insert(symbol.internal_name, symbol);
                    ret += fmt::format("decl @{}{}\n", symbol.internal_name,
                                       symbol.type->to_koopa());
                }
            }
            ret += "\n";

            for (const auto& item : declaration_or_function_items)
                ret += item->to_koopa();
            return ret;
        }
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
        ast_t return_type;
        std::string function_name;
        std::vector<ast_t> parameters;
        ast_t block;

    public:
        std::string to_koopa() const override;
    };

    /**
     * @brief AST of a parameter.
     * FuncFParam ::= BType IDENT ["[" "]" {"[" ConstExp "]"}];
     * Fixed:
     * FuncFParam ::= BType IDENT;
     * FuncFParam ::= BType IDENT "[" "]";
     * FuncFParam ::= BType IDENT "[" "]" ArrDimList;
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
     * Stmt ::= "return" ";";
     */
    class ast_statement_1_t : public ast_base_t
    {
    public:
        ast_t expression;

    public:
        std::string to_koopa() const override
        {
            std::string ret;
            if (expression)
            {
                if (auto const_value = expression->get_inline_number())
                    ret += fmt::format("    ret {}\n", *const_value);
                else
                {
                    ret += expression->to_koopa();
                    ret += fmt::format("    ret %{}\n",
                                       expression->get_result_id());
                }
            }
            else
                ret += fmt::format("    ret\n");
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
            auto type = std::dynamic_pointer_cast<type_function_t>(symbol.type);

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
            if (!(type->return_type->to_koopa().empty()))
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
            std::string ret;
            for (const auto& def : const_definitions)
                ret += def->to_koopa();
            return ret;
        }
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
        std::string to_koopa() const override;
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

    inline std::vector<int> generate_const_aggregate(
        ast_t ast_initial, const std::vector<size_t>& size_hint)
    {
        std::vector<int> ret;

        // 计算整个数组的大小。
        size_t whole_size = 1;
        for (auto size : size_hint)
            whole_size *= size; // TODO: 推导最高维大小。

        // 最外层总是一个列表。
        auto ast_list =
            std::dynamic_pointer_cast<ast_const_initial_value_2_t>(ast_initial);
        const auto& values = ast_list->const_initial_values;

        // 遍历列表。
        for (size_t i = 0; i < values.size(); i++)
        {
            if (auto ast_number =
                    std::dynamic_pointer_cast<ast_const_initial_value_1_t>(
                        values[i]))
            {
                // 遇到整数时,
                // 从当前待处理的维度中的最后一维开始填充数据。
                ret.push_back(*ast_number->get_inline_number());
            }
            else if (std::dynamic_pointer_cast<ast_const_initial_value_2_t>(
                         values[i]))
            {
                // 当前已经填充完毕的元素的个数必须是 len_n 的整数倍，
                // 否则这个初始化列表没有对齐数组维度的边界，
                // 你可以认为这种情况属于语义错误。
                if (ret.size() % size_hint.back())
                    throw std::domain_error("Invalid initializer list.");

                // 检查当前对齐到了哪一个边界，然后将当前初始化列表视作
                // 这个边界所对应的最长维度的数组的初始化列表，并递归处理。
                size_t cut_index = 1;
                size_t part_size = whole_size / size_hint[0];
                while (cut_index < size_hint.size())
                {
                    if (ret.size() % part_size)
                    {
                        part_size /= size_hint[cut_index];
                        cut_index++;
                    }
                    else
                        break;
                }
                auto part = generate_const_aggregate(
                    values[i], std::vector(size_hint.begin() + cut_index,
                                           size_hint.end()));
                ret.insert(ret.end(), part.begin(), part.end());
            }
            else
                throw std::domain_error("Unexpected type.");
        }

        // 填充 0。
        while (ret.size() < whole_size)
            ret.push_back(0);

        return ret;
    }
    inline std::string generate_const_aggregate_string(
        const std::vector<int>& flatten_aggregate,
        const std::vector<size_t>& size_hint)
    {
        std::string ret;

        if (std::all_of(flatten_aggregate.begin(), flatten_aggregate.end(),
                        [](int x) { return !x; }))
        {
            return "zeroinit";
        }

        if (size_hint.size() == 1)
        {
            for (int v : flatten_aggregate)
            {
                if (!ret.empty())
                    ret += ", ";
                ret += std::to_string(v);
            }
        }
        else
        {
            size_t part_size = 1;
            for (size_t i = 1; i < size_hint.size(); i++)
                part_size *= size_hint[i];
            for (size_t i = 0; i < size_hint[0]; i++)
            {
                if (!ret.empty())
                    ret += ", ";
                ret += generate_const_aggregate_string(
                    std::vector(flatten_aggregate.begin() + i * part_size,
                                flatten_aggregate.begin() +
                                    (i + 1) * part_size),
                    std::vector(size_hint.begin() + 1, size_hint.end()));
            }
        }

        return "{" + ret + "}";
    }

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
                symbol.type = type->type;
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
        std::string to_koopa() const override;
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

    inline std::vector<int> generate_variable_aggregate(
        ast_t ast_initial, const std::vector<size_t>& size_hint)
    {
        std::vector<int> ret;

        // 计算整个数组的大小。
        size_t whole_size = 1;
        for (auto size : size_hint)
            whole_size *= size; // TODO: 推导最高维大小。

        // 最外层总是一个列表。
        auto ast_list =
            std::dynamic_pointer_cast<ast_initial_value_2_t>(ast_initial);
        const auto& values = ast_list->initial_values;

        // 遍历列表。
        for (size_t i = 0; i < values.size(); i++)
        {
            if (auto ast_number =
                    std::dynamic_pointer_cast<ast_initial_value_1_t>(values[i]))
            {
                // 遇到整数时,
                // 从当前待处理的维度中的最后一维开始填充数据。
                ret.push_back(*ast_number->get_inline_number());
            }
            else if (std::dynamic_pointer_cast<ast_initial_value_2_t>(
                         values[i]))
            {
                // 当前已经填充完毕的元素的个数必须是 len_n 的整数倍，
                // 否则这个初始化列表没有对齐数组维度的边界，
                // 你可以认为这种情况属于语义错误。
                if (ret.size() % size_hint.back())
                    throw std::domain_error("Invalid initializer list.");

                // 检查当前对齐到了哪一个边界，然后将当前初始化列表视作
                // 这个边界所对应的最长维度的数组的初始化列表，并递归处理。
                size_t cut_index = 1;
                size_t part_size = whole_size / size_hint[0];
                while (cut_index < size_hint.size())
                {
                    if (ret.size() % part_size)
                    {
                        part_size /= size_hint[cut_index];
                        cut_index++;
                    }
                    else
                        break;
                }
                auto part = generate_variable_aggregate(
                    values[i], std::vector(size_hint.begin() + cut_index,
                                           size_hint.end()));
                ret.insert(ret.end(), part.begin(), part.end());
            }
            else
                throw std::domain_error("Unexpected type.");
        }

        // 填充 0。
        while (ret.size() < whole_size)
            ret.push_back(0);

        return ret;
    }
    inline std::vector<size_t> flatten_to_indices(
        size_t flatten_index, const std::vector<size_t>& size_hint)
    {
        std::vector<size_t> ret;

        size_t part_size = 1;
        for (size_t i = 1; i < size_hint.size(); i++)
            part_size *= size_hint[i];

        for (size_t i = 1; i < size_hint.size(); i++)
        {
            ret.push_back(flatten_index / part_size);
            flatten_index %= part_size;
            part_size /= size_hint[i];
        }
        ret.push_back(flatten_index);

        return ret;
    }
    inline std::string generate_variable_initialization_code(
        const std::string& internal_name, ast_t ast_initial,
        const std::vector<size_t>& size_hint,
        const std::vector<size_t>& original_size_hint, size_t& base_count)
    {
        std::string ret;

        // 计算整个数组的大小。
        size_t current_index = 0;
        size_t whole_size = 1;
        for (auto size : size_hint)
            whole_size *= size; // TODO: 推导最高维大小。

        // 最外层总是一个列表。
        auto ast_list =
            std::dynamic_pointer_cast<ast_initial_value_2_t>(ast_initial);
        const auto& values = ast_list->initial_values;

        // 遍历列表。
        for (size_t i = 0; i < values.size(); i++)
        {
            if (auto ast_number =
                    std::dynamic_pointer_cast<ast_initial_value_1_t>(values[i]))
            {
                // 遇到整数时,
                // 从当前待处理的维度中的最后一维开始填充数据。
                std::string expression_holder;

                if (auto const_value = ast_number->get_inline_number())
                    expression_holder = std::to_string(*const_value);
                else
                {
                    ret += ast_number->to_koopa();
                    expression_holder =
                        fmt::format("%{}", ast_number->get_result_id());
                }

                int current_id = 0;
                std::string current_source = fmt::format("@{}", internal_name);
                auto indices =
                    flatten_to_indices(base_count, original_size_hint);
                for (size_t i = 0; i < indices.size(); i++)
                {
                    current_id = new_result_id();
                    ret += fmt::format("    %{} = getelemptr {}, {}\n",
                                       current_id, current_source, indices[i]);
                    current_source = fmt::format("%{}", current_id);
                }
                ret += fmt::format("    store {}, {}\n", expression_holder,
                                   current_source);

                base_count++;
                current_index++;
            }
            else if (std::dynamic_pointer_cast<ast_initial_value_2_t>(
                         values[i]))
            {
                // 当前已经填充完毕的元素的个数必须是 len_n 的整数倍，
                // 否则这个初始化列表没有对齐数组维度的边界，
                // 你可以认为这种情况属于语义错误。
                if (ret.size() % size_hint.back())
                    throw std::domain_error("Invalid initializer list.");

                // 检查当前对齐到了哪一个边界，然后将当前初始化列表视作
                // 这个边界所对应的最长维度的数组的初始化列表，并递归处理。
                size_t cut_index = 1;
                size_t part_size = whole_size / size_hint[0];
                while (cut_index < size_hint.size())
                {
                    if (ret.size() % part_size)
                    {
                        part_size /= size_hint[cut_index];
                        cut_index++;
                    }
                    else
                        break;
                }
                auto part = generate_variable_initialization_code(
                    internal_name, values[i],
                    std::vector(size_hint.begin() + cut_index, size_hint.end()),
                    original_size_hint, base_count);

                base_count += part_size;
                current_index += part_size;

                ret += part;
            }
            else
                throw std::domain_error("Unexpected type.");
        }

        // 填充 0。
        while (current_index < whole_size)
        {
            std::string expression_holder = "0";

            int current_id = 0;
            std::string current_source = fmt::format("@{}", internal_name);
            auto indices = flatten_to_indices(base_count, original_size_hint);
            for (size_t i = 0; i < indices.size(); i++)
            {
                current_id = new_result_id();
                ret += fmt::format("    %{} = getelemptr {}, {}\n", current_id,
                                   current_source, indices[i]);
                current_source = fmt::format("%{}", current_id);
            }
            ret += fmt::format("    store {}, {}\n", expression_holder,
                               current_source);

            base_count++;
            current_index++;
        }

        return ret;
    }

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
            if (std::get<symbol_const_t>(*symbol).type->get_base_type())
                return std::nullopt; // 是数组。

            return std::get<symbol_const_t>(*symbol).value;
        }

    public:
        std::string to_koopa() const override
        {
            std::string ret;

            if (std::holds_alternative<symbol_variable_t>(*st.at(raw_name)))
            {
                auto symbol = std::get<symbol_variable_t>(*st.at(raw_name));
                auto current_type = symbol.type;

                int current_id = 0;
                std::string current_source =
                    fmt::format("@{}", symbol.internal_name);
                for (size_t i = 0; i < indices.size(); i++)
                {
                    auto expression = indices[i];
                    std::string inst;
                    if (std::dynamic_pointer_cast<type_pointer_t>(current_type))
                        inst = "getptr";
                    else
                        inst = "getelemptr";

                    if (inst == "getptr")
                    {
                        current_id = new_result_id();
                        ret += fmt::format("    %{} = load {}\n", current_id,
                                           current_source);
                        current_source = fmt::format("%{}", current_id);
                    }

                    if (auto const_value = expression->get_inline_number())
                    {
                        current_id = new_result_id();
                        ret += fmt::format("    %{} = {} {}, {}\n", current_id,
                                           inst, current_source, *const_value);
                    }
                    else
                    {
                        ret += expression->to_koopa();
                        current_id = new_result_id();
                        ret += fmt::format("    %{} = {} {}, %{}\n", current_id,
                                           inst, current_source,
                                           expression->get_result_id());
                    }
                    current_source = fmt::format("%{}", current_id);
                    current_type = current_type->get_base_type();
                }

                assign_result_id();
                if (current_type->get_base_type()) // 转换为指针。
                {
                    if (std::dynamic_pointer_cast<type_array_t>(
                            current_type)) // 数组隐式转换为指针。
                    {
                        ret += fmt::format("    %{} = getelemptr {}, 0\n",
                                           get_result_id(), current_source);
                    }
                    else // 直接传递指针。
                    {
                        ret += fmt::format("    %{} = load {}\n",
                                           get_result_id(), current_source);
                    }
                }
                else
                    ret += fmt::format("    %{} = load {}\n", get_result_id(),
                                       current_source);
            }
            else if (std::holds_alternative<symbol_const_t>(*st.at(raw_name)))
            {
                auto symbol = std::get<symbol_const_t>(*st.at(raw_name));

                auto current_type = symbol.type;

                int current_id = 0;
                std::string current_source =
                    fmt::format("@{}", symbol.internal_name);
                for (size_t i = 0; i < indices.size(); i++)
                {
                    auto expression = indices[i];
                    std::string inst;
                    if (std::dynamic_pointer_cast<type_pointer_t>(current_type))
                        inst = "getptr";
                    else
                        inst = "getelemptr";

                    if (inst == "getptr")
                    {
                        current_id = new_result_id();
                        ret += fmt::format("    %{} = load {}\n", current_id,
                                           current_source);
                        current_source = fmt::format("%{}", current_id);
                    }

                    if (auto const_value = expression->get_inline_number())
                    {
                        current_id = new_result_id();
                        ret += fmt::format("    %{} = {} {}, {}\n", current_id,
                                           inst, current_source, *const_value);
                    }
                    else
                    {
                        ret += expression->to_koopa();
                        current_id = new_result_id();
                        ret += fmt::format("    %{} = {} {}, %{}\n", current_id,
                                           inst, current_source,
                                           expression->get_result_id());
                    }
                    current_source = fmt::format("%{}", current_id);
                    current_type = current_type->get_base_type();
                }

                assign_result_id();
                if (current_type->get_base_type()) // 转换为指针。
                {
                    if (std::dynamic_pointer_cast<type_array_t>(
                            current_type)) // 数组隐式转换为指针。
                    {
                        ret += fmt::format("    %{} = getelemptr {}, 0\n",
                                           get_result_id(), current_source);
                    }
                    else // 直接传递指针。
                    {
                        ret += fmt::format("    %{} = load {}\n",
                                           get_result_id(), current_source);
                    }
                }
                else
                    ret += fmt::format("    %{} = load {}\n", get_result_id(),
                                       current_source);
            }

            return ret;
        }
    };

    inline std::string ast_const_definition_t::to_koopa() const
    {
        std::string ret;

        symbol_const_t symbol;
        symbol.type = type->type;

        if (symbol.type->get_base_type()) // 是数组，计算出列表，生成代码。
        {
            // 生成数组大小。
            std::vector<size_t> size_hint;
            {
                auto t = symbol.type;
                while (t->get_base_type())
                {
                    size_hint.push_back(
                        std::dynamic_pointer_cast<type_array_t>(t)->array_size);
                    t = t->get_base_type();
                }
            }

            // 递归处理初始化列表。
            auto aggregate =
                generate_const_aggregate(const_initial_value, size_hint);

            // 直接存入符号表，并在之后利用符号表判断全局性。
            st.insert(raw_name, std::move(symbol));
            symbol = std::get<symbol_const_t>(*st.at(raw_name));

            // 生成代码。
            std::string initial_value_string =
                generate_const_aggregate_string(aggregate, size_hint);
            std::string prefix;
            {
                if (st.is_global(raw_name))
                    prefix = "global ";
                else
                    prefix = "    ";
            }

            ret += fmt::format("{}@{} = alloc {}", // The line does not end.
                               prefix, symbol.internal_name,
                               symbol.type->to_koopa());
            if (st.is_global(raw_name))
                ret += fmt::format(", {}\n", initial_value_string);
            else
            {
                ret += "\n";
                ret += fmt::format("    store {}, @{}\n", initial_value_string,
                                   symbol.internal_name);
            }
        }
        else // 是一个常数，直接计算出内联数，并存入符号。
        {
            symbol.value = *const_initial_value->get_inline_number();
            st.insert(raw_name, std::move(symbol));
        }

        return ret;
    }

    inline std::string ast_variable_definition_2_t::to_koopa() const
    {
        auto ret = ast_variable_definition_t::to_koopa();

        auto symbol = std::get<symbol_variable_t>(*st.at(raw_name));

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

        if (type->type->get_base_type()) // 是数组。
        {
            // 生成数组大小。
            std::vector<size_t> size_hint;
            {
                auto t = symbol.type;
                while (t->get_base_type())
                {
                    size_hint.push_back(
                        std::dynamic_pointer_cast<type_array_t>(t)->array_size);
                    t = t->get_base_type();
                }
            }

            if (st.is_global(raw_name))
            {
                // 递归处理初始化列表。
                auto aggregate =
                    generate_variable_aggregate(initial_value, size_hint);

                // 生成代码。
                std::string initial_value_string =
                    generate_const_aggregate_string(aggregate, size_hint);
                ret += fmt::format("{}\n\n", initial_value_string);
            }
            else
            {
                // 递归生成代码。
                size_t base_count{};
                ret += generate_variable_initialization_code(
                    symbol.internal_name, initial_value, size_hint, size_hint,
                    base_count);
            }
        }
        else // 是单个数。
        {
            if (st.is_global(raw_name))
            {
                // 只允许使用常量表达式初始化全局变量。
                assert(const_initial_value);
                ret += fmt::format("{}\n\n", *const_initial_value);
            }
            else
            {
                ret += fmt::format("    store {}, @{}\n", initial_value_holder,
                                   symbol.internal_name);
            }
        }

        return ret;
    }

    inline std::string ast_function_t::to_koopa() const
    {
        std::string ret;

        // Insert the function into symbol table.
        {
            symbol_function_t symbol;
            auto type = std::make_shared<type_function_t>();
            type->return_type =
                std::dynamic_pointer_cast<ast_type_t>(return_type)->type;
            symbol.type = type;
            // TODO: Set the parameter types.
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
            return_type_string = return_type->to_koopa();
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
            {
                symbol_variable_t symbol;
                symbol.type =
                    std::dynamic_pointer_cast<ast_type_t>(param->type)->type;
                st.insert(param->raw_name, symbol);
            }
            auto symbol = std::get<symbol_variable_t>(*st.at(param->raw_name));

            ret += fmt::format("    @{} = alloc {}\n", symbol.internal_name,
                               param->type->to_koopa());
            ret += fmt::format("    store @{}, @{}\n", param->raw_name,
                               symbol.internal_name);
        }

        ret += block->to_koopa();
        if (return_type->to_koopa().empty())
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

        {
            auto ast_lvalue = std::dynamic_pointer_cast<ast_lvalue_t>(lvalue);
            auto symbol =
                std::get<symbol_variable_t>(*st.at(ast_lvalue->raw_name));
            auto current_type = symbol.type;

            int current_id = 0;
            std::string current_source =
                fmt::format("@{}", symbol.internal_name);
            for (size_t i = 0; i < ast_lvalue->indices.size(); i++)
            {
                auto expression = ast_lvalue->indices[i];
                std::string inst;
                if (std::dynamic_pointer_cast<type_pointer_t>(current_type))
                    inst = "getptr";
                else
                    inst = "getelemptr";

                if (inst == "getptr")
                {
                    current_id = new_result_id();
                    ret += fmt::format("    %{} = load {}\n", current_id,
                                       current_source);
                    current_source = fmt::format("%{}", current_id);
                }

                if (auto const_value = expression->get_inline_number())
                {
                    current_id = new_result_id();
                    ret += fmt::format("    %{} = {} {}, {}\n", current_id,
                                       inst, current_source, *const_value);
                }
                else
                {
                    ret += expression->to_koopa();
                    current_id = new_result_id();
                    ret += fmt::format("    %{} = {} {}, %{}\n", current_id,
                                       inst, current_source,
                                       expression->get_result_id());
                }
                current_source = fmt::format("%{}", current_id);
                current_type = current_type->get_base_type();
            }

            ret += fmt::format("    store {}, {}\n", expression_holder,
                               current_source);
        }
        return ret;
    }
} // namespace compiler::ast
