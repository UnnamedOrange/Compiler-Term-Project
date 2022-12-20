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
#include <frontend/ast.h>
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

%token INT RETURN CONST IF ELSE WHILE BREAK CONTINUE
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

%type nt_program nt_function nt_function_type
%type nt_number
%type nt_block
%type nt_block_item nt_block_item_list
%type nt_statement
%type nt_matched_statement nt_open_statement
%type nt_declaration nt_const_declaration nt_variable_declaration
%type nt_base_type
%type nt_lvalue
%type nt_const_definition nt_const_definition_list
%type nt_const_initial_value nt_const_expression
%type nt_variable_definition nt_variable_definition_list
%type nt_initial_value
%type nt_expression nt_primary_expression
%type nt_unary_expression nt_unary_operator
%type nt_multiply_expression nt_multiply_operator
%type nt_add_expression nt_add_operator
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
}
nt_function : nt_function_type IDENTIFIER '(' ')' nt_block {
    auto ast_function = std::make_shared<ast_function_t>();
    ast_function->function_type = std::get<ast_t>($1);
    ast_function->function_name = std::get<string>($2);
    ast_function->block = std::get<ast_t>($5);
    $$ = ast_function;
}
nt_function_type : INT {
    auto ast_function_type = std::make_shared<ast_function_type_t>();
    ast_function_type->type_name = "int";
    $$ = ast_function_type;
}
nt_block : '{' '}' {
    $$ = std::make_shared<ast_block_t>();
}
| '{' nt_block_item_list '}' {
    auto ast_block = std::make_shared<ast_block_t>();
    auto current_list = std::dynamic_pointer_cast<ast_block_item_list_t>(std::get<ast_t>($2));
    while (current_list)
    {
        ast_block->block_items.push_back(std::move(current_list->block_item));
        current_list = current_list->block_item_list;
    }
    $$ = ast_block;
}
nt_block_item_list : nt_block_item {
    auto ast_block_item_list = std::make_shared<ast_block_item_list_t>();
    ast_block_item_list->block_item = std::get<ast_t>($1);
    $$ = ast_block_item_list;
}
| nt_block_item nt_block_item_list {
    auto ast_block_item_list = std::make_shared<ast_block_item_list_t>();
    ast_block_item_list->block_item = std::get<ast_t>($1);
    ast_block_item_list->block_item_list = std::dynamic_pointer_cast<ast_block_item_list_t>(std::get<ast_t>($2));
    $$ = ast_block_item_list;
}
nt_block_item : nt_declaration | nt_statement {
    auto ast_block_item = std::make_shared<ast_block_item_t>();
    ast_block_item->item = std::get<ast_t>($1);
    $$ = ast_block_item;
}
nt_statement : nt_matched_statement | nt_open_statement {
    $$ = $1;
}
nt_open_statement : IF '(' nt_expression ')' nt_statement {
    auto ast_statement = std::make_shared<ast_statement_5_t>();
    ast_statement->condition_expression = std::get<ast_t>($3);
    ast_statement->if_branch = std::get<ast_t>($5);
    $$ = ast_statement;
}
| IF '(' nt_expression ')' nt_matched_statement ELSE nt_open_statement {
    auto ast_statement = std::make_shared<ast_statement_5_t>();
    ast_statement->condition_expression = std::get<ast_t>($3);
    ast_statement->if_branch = std::get<ast_t>($5);
    ast_statement->else_branch = std::get<ast_t>($7);
    $$ = ast_statement;
}
nt_matched_statement : RETURN nt_expression ';' {
    auto ast_statement = std::make_shared<ast_statement_1_t>();
    ast_statement->expression = std::get<ast_t>($2);
    $$ = ast_statement;
}
| nt_lvalue '=' nt_expression ';' {
    auto ast_statement = std::make_shared<ast_statement_2_t>();
    ast_statement->lvalue = std::get<ast_t>($1);
    ast_statement->expression = std::get<ast_t>($3);
    $$ = ast_statement;
}
| nt_expression ';' {
    auto ast_statement = std::make_shared<ast_statement_3_t>();
    ast_statement->expression = std::get<ast_t>($1);
    $$ = ast_statement;
}
| ';' {
    auto ast_statement = std::make_shared<ast_statement_3_t>();
    $$ = ast_statement;
}
| nt_block {
    auto ast_statement = std::make_shared<ast_statement_4_t>();
    ast_statement->block = std::get<ast_t>($1);
    $$ = ast_statement;
}
| IF '(' nt_expression ')' nt_matched_statement ELSE nt_matched_statement {
    auto ast_statement = std::make_shared<ast_statement_5_t>();
    ast_statement->condition_expression = std::get<ast_t>($3);
    ast_statement->if_branch = std::get<ast_t>($5);
    ast_statement->else_branch = std::get<ast_t>($7);
    $$ = ast_statement;
}
nt_number : INT_LITERAL {
    $$ = $1;
}
nt_expression : nt_lor_expression {
    auto ast_expression = std::make_shared<ast_expression_t>();
    ast_expression->lor_expression = std::get<ast_t>($1);
    $$ = ast_expression;
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
| nt_lvalue {
    auto ast_primary_expression = std::make_shared<ast_primary_expression_3_t>();
    ast_primary_expression->lvalue = std::get<ast_t>($1);
    $$ = ast_primary_expression;
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
nt_declaration : nt_const_declaration {
    auto ast_declaration = std::make_shared<ast_declaration_1_t>();
    ast_declaration->const_declaration = std::get<ast_t>($1);
    $$ = ast_declaration;
}
| nt_variable_declaration {
    auto ast_declaration = std::make_shared<ast_declaration_2_t>();
    ast_declaration->variable_declaration = std::get<ast_t>($1);
    $$ = ast_declaration;
}
nt_base_type : INT {
    auto ast_base_type = std::make_shared<ast_base_type_t>();
    ast_base_type->type_name = "int";
    $$ = ast_base_type;
}
nt_const_declaration : CONST nt_base_type nt_const_definition_list ';' {
    auto ast_const_definition = std::make_shared<ast_const_declaration_t>();
    ast_const_definition->base_type = std::get<ast_t>($2);
    auto current_list = std::dynamic_pointer_cast<ast_const_definition_list_t>(std::get<ast_t>($3));
    while (current_list)
    {
        auto def = std::dynamic_pointer_cast<ast_const_definition_t>(std::move(current_list->const_definition));
        def->base_type = std::dynamic_pointer_cast<ast_base_type_t>(ast_const_definition->base_type);
        ast_const_definition->const_definitions.push_back(std::move(def));
        current_list = current_list->const_definition_list;
    }
    $$ = ast_const_definition;
}
nt_const_definition_list : nt_const_definition {
    auto ast_const_definition_list = std::make_shared<ast_const_definition_list_t>();
    ast_const_definition_list->const_definition = std::get<ast_t>($1);
    $$ = ast_const_definition_list;
}
| nt_const_definition ',' nt_const_definition_list {
    auto ast_const_definition_list = std::make_shared<ast_const_definition_list_t>();
    ast_const_definition_list->const_definition = std::get<ast_t>($1);
    ast_const_definition_list->const_definition_list = std::dynamic_pointer_cast<ast_const_definition_list_t>(std::get<ast_t>($3));
    $$ = ast_const_definition_list;
}
nt_const_definition : IDENTIFIER '=' nt_const_initial_value {
    auto ast_const_definition = std::make_shared<ast_const_definition_t>();
    ast_const_definition->raw_name = std::get<string>($1);
    ast_const_definition->const_initial_value = std::get<ast_t>($3);
    $$ = ast_const_definition;
}
nt_const_initial_value : nt_const_expression {
    auto ast_const_initial_value = std::make_shared<ast_const_initial_value_t>();
    ast_const_initial_value->const_expression = std::get<ast_t>($1);
    $$ = ast_const_initial_value;
}
nt_const_expression : nt_expression {
    auto ast_const_expression = std::make_shared<ast_const_expression_t>();
    ast_const_expression->expression = std::get<ast_t>($1);
    $$ = ast_const_expression;
}
nt_variable_declaration : nt_base_type nt_variable_definition_list ';' {
    auto ast_variable_definition = std::make_shared<ast_variable_declaration_t>();
    ast_variable_definition->base_type = std::get<ast_t>($1);
    auto current_list = std::dynamic_pointer_cast<ast_variable_definition_list_t>(std::get<ast_t>($2));
    while (current_list)
    {
        auto def = std::dynamic_pointer_cast<ast_variable_definition_t>(std::move(current_list->variable_definition));
        def->base_type = std::dynamic_pointer_cast<ast_base_type_t>(ast_variable_definition->base_type);
        ast_variable_definition->variable_definitions.push_back(std::move(def));
        current_list = current_list->variable_definition_list;
    }
    $$ = ast_variable_definition;
}
nt_variable_definition_list : nt_variable_definition {
    auto ast_variable_definition_list = std::make_shared<ast_variable_definition_list_t>();
    ast_variable_definition_list->variable_definition = std::get<ast_t>($1);
    $$ = ast_variable_definition_list;
}
| nt_variable_definition ',' nt_variable_definition_list {
    auto ast_variable_definition_list = std::make_shared<ast_variable_definition_list_t>();
    ast_variable_definition_list->variable_definition = std::get<ast_t>($1);
    ast_variable_definition_list->variable_definition_list = std::dynamic_pointer_cast<ast_variable_definition_list_t>(std::get<ast_t>($3));
    $$ = ast_variable_definition_list;
}
nt_variable_definition : IDENTIFIER {
    auto ast_variable_definition = std::make_shared<ast_variable_definition_1_t>();
    ast_variable_definition->raw_name = std::get<string>($1);
    $$ = ast_variable_definition;
}
| IDENTIFIER '=' nt_initial_value {
    auto ast_variable_definition = std::make_shared<ast_variable_definition_2_t>();
    ast_variable_definition->raw_name = std::get<string>($1);
    ast_variable_definition->initial_value = std::get<ast_t>($3);
    $$ = ast_variable_definition;
}
nt_initial_value : nt_expression {
    auto ast_initial_value = std::make_shared<ast_initial_value_t>();
    ast_initial_value->expression = std::get<ast_t>($1);
    $$ = ast_initial_value;
}
nt_lvalue : IDENTIFIER {
    auto ast_lvalue = std::make_shared<ast_lvalue_t>();
    ast_lvalue->raw_name = std::get<string>($1);
    $$ = ast_lvalue;
}
%%

/* 第四部分：辅助函数 */
void yyerror(ast_t& ast, const char* s)
{
    std::cerr << fmt::format("[Error] YACC: {}.", s) << std::endl;
}
