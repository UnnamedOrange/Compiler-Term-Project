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

/* 第二部分（二）：动作 */

/* 形如：
 * <正则表达式> {动作}
 *
 * 例如：
 * {Identifier} { yylval.str_val = yytext; return IDENT; }
 */

%%
%%

/* 第三部分：辅助函数 */
