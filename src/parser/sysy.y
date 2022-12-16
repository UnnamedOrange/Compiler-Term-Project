/**
 * @file sysy.y
 * @author UnnamedOrange
 * @brief Parser for SysY.
 *
 * @copyright Copyright (c) UnnamedOrange. Licensed under the MIT License.
 * See the LICENSE file in the repository root for full license text.
 */

/* 第一部分（一）：C++ 导言程序 */
%{
// 包含头文件。
#include <iostream>

#include <fmt/core.h>
%}

/* 第一部分（二）：自定义代码块 */
// https://www.gnu.org/software/bison/manual/html_node/_0025code-Summary.html
%code requires {
// 定义类型。
#include <ast/ast.h>
using namespace compiler::ast;

#include <variant>
using string = std::string;
using symbol_type = std::variant<ast_t, string, int>;

#define YYSTYPE symbol_type

// 声明词法分析外部函数。YACC 默认使用 yylex。
int yylex();

// 前向声明错误处理函数。其中参数一默认是 parse-param。
void yyerror(ast_t& ast, const char* s);
}

/* 第二部分（一）：起始符号翻译结果定义 */

// 起始符号翻译结果以函数参数的形式存在，
// 在起始符号的产生式动作中对该参数进行赋值，
// 实现翻译结果的返回。
%parse-param { ast_t& ast }

/* 第二部分（二）：类型定义 */

// 词法单元联合体 yylval 的类型定义。可以定义为 C 语言的联合体 union。
/*
%union {
    ast_t* ast_val;
    string* str_val;
    int int_val;
}
*/
// 也可以用宏进行简单定义，没有显式写出的符号都是 YYSTYPE 类型。
// 在 %code requires 中定义。
/*
#define YYSTYPE string
*/
// 还可以使用 variant。
// https://www.gnu.org/software/bison/manual/bison.html#C_002b_002b-Variants
/*
%define api.value.type variant
*/
// 考虑到编译选项不可修改，此处使用宏定义的 variant。

/* 第二部分（三）：终结符定义 */

/* 形如：
 * %token <终结符枚举名> { <终结符枚举名> ... } // 类型默认为 YYSTYPE。
 * %token <<<类型>>> { <终结符枚举名> ... } // 类型在 union 中定义。
 */

%token INT RETURN
%token IDENTIFIER
%token INT_LITERAL
%token LT GT LE GE EQ NE
%token LAND LOR

/* 第二部分（四）：非终结符类型定义 */

/* 形如：
 * %type <非终结符名> { <非终结符名> ... } // 类型默认为 YYSTYPE。
 * %type <<<类型>>> { <非终结符名> ... } // 类型在 union 中定义。
 *
 * 如果使用默认类型，则可省略。
 */

%type nt_program nt_function nt_function_type nt_block nt_statement nt_number
%type nt_expression nt_primary_expression nt_unary_expression nt_unary_operator
%type nt_multiply_expression nt_multiply_operator nt_add_expression nt_add_operator
%type nt_relation_expression nt_relation_operator
%type nt_equation_expression nt_equation_operator
%type nt_land_expression
%type nt_lor_expression

/* 第三部分：动作 */

/* 形如：
 * <非终结符> : <产生式> { <动作> };
 *
 * 动作中，$$ 表示产生式左侧符号的数据，$1 表示产生式右侧第 1 个符号的数据。
 */

%%
nt_program : nt_function {
    // 样例：
    // symbol_type s_int = $1;
    // $$ = std::make_shared<ast_program_t>();
    // ast = std::move(std::get<ast_t>($$));
    auto ast_temp = std::make_shared<ast_program_t>();
    ast_temp->function = std::get<ast_t>($1);
    ast = std::move(ast_temp); // See parse-param.
};
nt_function : nt_function_type IDENTIFIER '(' ')' nt_block {
    auto ast_function = std::make_shared<ast_function_t>();
    ast_function->function_type = std::get<ast_t>($1);
    ast_function->function_name = std::get<string>($2);
    ast_function->block = std::get<ast_t>($5);
    $$ = ast_function;
};
nt_function_type : INT {
    auto ast_function_type = std::make_shared<ast_function_type_t>();
    ast_function_type->type_name = "int";
    $$ = ast_function_type;
};
nt_block : '{' nt_statement '}' {
    auto ast_block = std::make_shared<ast_block_t>();
    ast_block->statement = std::get<ast_t>($2);
    $$ = ast_block;
};
nt_statement : RETURN nt_expression ';' {
    auto ast_statement = std::make_shared<ast_statement_t>();
    ast_statement->expression = std::get<ast_t>($2);
    $$ = ast_statement;
};
nt_number : INT_LITERAL {
    $$ = $1;
};
nt_expression : nt_lor_expression {
    auto ast_expression = std::make_shared<ast_expression_t>();
    ast_expression->lor_expression = std::get<ast_t>($1);
    $$ = ast_expression;
}
nt_unary_expression : nt_primary_expression {
    auto ast_unary_expression = std::make_shared<ast_unary_expression_1_t>();
    ast_unary_expression->primary_expression = std::get<ast_t>($1);
    $$ = ast_unary_expression;
}
| nt_unary_operator nt_unary_expression {
    auto ast_unary_expression = std::make_shared<ast_unary_expression_2_t>();
    ast_unary_expression->op = std::get<string>($1);
    ast_unary_expression->unary_expression = std::get<ast_t>($2);
    $$ = ast_unary_expression;
}
nt_primary_expression : '(' nt_expression ')' {
    auto ast_primary_expression = std::make_shared<ast_primary_expression_1_t>();
    ast_primary_expression->expression = std::get<ast_t>($2);
    $$ = ast_primary_expression;
}
| nt_number {
    auto ast_primary_expression = std::make_shared<ast_primary_expression_2_t>();
    ast_primary_expression->number = std::get<int>($1);
    $$ = ast_primary_expression;
}
nt_unary_operator : '+' | '-' | '!' {
    $$ = $1;
}
nt_multiply_expression : nt_unary_expression {
    auto ast_multiply_expression = std::make_shared<ast_multiply_expression_1_t>();
    ast_multiply_expression->unary_expression = std::get<ast_t>($1);
    $$ = ast_multiply_expression;
}
| nt_multiply_expression nt_multiply_operator nt_unary_expression {
    auto ast_multiply_expression = std::make_shared<ast_multiply_expression_2_t>();
    ast_multiply_expression->multiply_expression = std::get<ast_t>($1);
    ast_multiply_expression->op = std::get<string>($2);
    ast_multiply_expression->unary_expression = std::get<ast_t>($3);
    $$ = ast_multiply_expression;
}
nt_multiply_operator : '*' | '/' | '%' {
    $$ = $1;
}
nt_add_expression : nt_multiply_expression {
    auto ast_add_expression = std::make_shared<ast_add_expression_1_t>();
    ast_add_expression->multiply_expression = std::get<ast_t>($1);
    $$ = ast_add_expression;
}
| nt_add_expression nt_add_operator nt_multiply_expression {
    auto ast_add_expression = std::make_shared<ast_add_expression_2_t>();
    ast_add_expression->add_expression = std::get<ast_t>($1);
    ast_add_expression->op = std::get<string>($2);
    ast_add_expression->multiply_expression = std::get<ast_t>($3);
    $$ = ast_add_expression;
}
nt_add_operator : '+' | '-' {
    $$ = $1;
}
nt_relation_expression : nt_add_expression {
    auto ast_relation_expression = std::make_shared<ast_relation_expression_1_t>();
    ast_relation_expression->add_expression = std::get<ast_t>($1);
    $$ = ast_relation_expression;
}
| nt_relation_expression nt_relation_operator nt_add_expression {
    auto ast_relation_expression = std::make_shared<ast_relation_expression_2_t>();
    ast_relation_expression->relation_expression = std::get<ast_t>($1);
    ast_relation_expression->op = std::get<string>($2);
    ast_relation_expression->add_expression = std::get<ast_t>($3);
    $$ = ast_relation_expression;
}
nt_relation_operator : LT | GT | LE | GE {
    $$ = $1;
}
nt_equation_expression : nt_relation_expression {
    auto ast_equation_expression = std::make_shared<ast_equation_expression_1_t>();
    ast_equation_expression->relation_expression = std::get<ast_t>($1);
    $$ = ast_equation_expression;
}
| nt_equation_expression nt_equation_operator nt_relation_expression {
    auto ast_equation_expression = std::make_shared<ast_equation_expression_2_t>();
    ast_equation_expression->equation_expression = std::get<ast_t>($1);
    ast_equation_expression->op = std::get<string>($2);
    ast_equation_expression->relation_expression = std::get<ast_t>($3);
    $$ = ast_equation_expression;
}
nt_equation_operator : EQ | NE {
    $$ = $1;
}
nt_land_expression : nt_equation_expression {
    auto ast_land_expression = std::make_shared<ast_land_expression_1_t>();
    ast_land_expression->equation_expression = std::get<ast_t>($1);
    $$ = ast_land_expression;
}
| nt_land_expression LAND nt_equation_expression {
    auto ast_land_expression = std::make_shared<ast_land_expression_2_t>();
    ast_land_expression->land_expression = std::get<ast_t>($1);
    ast_land_expression->equation_expression = std::get<ast_t>($3);
    $$ = ast_land_expression;
}
nt_lor_expression : nt_land_expression {
    auto ast_lor_expression = std::make_shared<ast_lor_expression_1_t>();
    ast_lor_expression->land_expression = std::get<ast_t>($1);
    $$ = ast_lor_expression;
}
| nt_lor_expression LOR nt_land_expression {
    auto ast_lor_expression = std::make_shared<ast_lor_expression_2_t>();
    ast_lor_expression->lor_expression = std::get<ast_t>($1);
    ast_lor_expression->land_expression = std::get<ast_t>($3);
    $$ = ast_lor_expression;
}
%%

/* 第四部分：辅助函数 */
void yyerror(ast_t& ast, const char* s)
{
    std::cerr << fmt::format("[Error] YACC: {}.", s) << std::endl;
}
