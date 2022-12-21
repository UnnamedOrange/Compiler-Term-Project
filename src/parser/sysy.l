/**
 * @file sysy.l
 * @author UnnamedOrange
 * @brief Lexer for SysY.
 *
 * @copyright Copyright (c) UnnamedOrange. Licensed under the MIT License.
 * See the LICENSE file in the repository root for full license text.
 */

/* 第零部分：Lex 生成选项 */
%option noyywrap
%option nounput
%option noinput

/* 第一部分：C++ 开头程序 */
%{
#include <cstdlib>
#include <string>

#include "sysy.tab.hpp" // 使用 Bison 中关于 token 的定义。
%}

/* 第二部分（零）：状态定义 */

/* 形如：
 * %s <状态名>
 */

/* 第二部分（一）：正则表达式定义 */

/* 形如：
 * <名字> <正则表达式>
 */

WhiteSpace    [ \t\n\r]*
LineComment   "//".*
BlockComment  "/\*"([^*]|\*+[^*/])*\*+"/"

Identifier    [a-zA-Z_][a-zA-Z0-9_]*

Decimal       [1-9][0-9]*
Octal         0[0-7]*
Hexadecimal   0[xX][0-9a-fA-F]+

/* 第二部分（二）：动作 */

/* 形如：
 * <正则表达式> {动作}
 *
 * 例如：
 * {Identifier} { yylval.str_val = yytext; return IDENT; }
 */

%%

{WhiteSpace}    { /* 忽略, 不做任何操作 */ }
{LineComment}   { /* 忽略, 不做任何操作 */ }
{BlockComment}  { /* 忽略, 不做任何操作 */ }

"int"           { yylval = std::string(yytext); return INT; }
"void"          { yylval = std::string(yytext); return VOID; }
"return"        { yylval = std::string(yytext); return RETURN; }
"const"         { yylval = std::string(yytext); return CONST; }
"if"            { yylval = std::string(yytext); return IF; }
"else"          { yylval = std::string(yytext); return ELSE; }
"while"         { yylval = std::string(yytext); return WHILE; }
"break"         { yylval = std::string(yytext); return BREAK; }
"continue"      { yylval = std::string(yytext); return CONTINUE; }

"<"             { yylval = std::string(yytext); return LT; }
">"             { yylval = std::string(yytext); return GT; }
"<="            { yylval = std::string(yytext); return LE; }
">="            { yylval = std::string(yytext); return GE; }
"=="            { yylval = std::string(yytext); return EQ; }
"!="            { yylval = std::string(yytext); return NE; }

"&&"            { yylval = std::string(yytext); return LAND; }
"||"            { yylval = std::string(yytext); return LOR; }

{Identifier}    { yylval = std::string(yytext); return IDENTIFIER; }

{Decimal}       { yylval = static_cast<int>(std::strtol(yytext, nullptr, 0)); return INT_LITERAL; }
{Octal}         { yylval = static_cast<int>(std::strtol(yytext, nullptr, 0)); return INT_LITERAL; }
{Hexadecimal}   { yylval = static_cast<int>(std::strtol(yytext, nullptr, 0)); return INT_LITERAL; }

.               { yylval = std::string(yytext); return yytext[0]; }

%%

/* 第三部分：辅助函数 */
