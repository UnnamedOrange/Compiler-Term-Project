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
using symbol_type = std::variant<ast_t, int, double, string>;

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

%token INT

/* 第二部分（四）：非终结符类型定义 */

/* 形如：
 * %type <非终结符名> { <非终结符名> ... } // 类型默认为 YYSTYPE。
 * %type <<<类型>>> { <非终结符名> ... } // 类型在 union 中定义。
 *
 * 如果使用默认类型，则可省略。
 */

/* 第三部分：动作 */

/* 形如：
 * <非终结符> : <产生式> { <动作> };
 *
 * 动作中，$$ 表示产生式左侧符号的数据，$1 表示产生式右侧第 1 个符号的数据。
 */

%%
nt_program : INT {
    // 样例：
    // symbol_type s_int = $1;
    // $$ = std::make_shared<ast_program_t>();
    // ast = std::move(std::get<ast_t>($$));
};
%%

/* 第四部分：辅助函数 */
void yyerror(ast_t& ast, const char* s)
{
    std::cerr << fmt::format("[Error] YACC: {}.", s) << std::endl;
}
