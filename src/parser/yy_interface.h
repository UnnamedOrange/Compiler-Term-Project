/**
 * @file yy_interface.h
 * @author UnnamedOrange
 * @brief Lex and YACC interface.
 *
 * @copyright Copyright (c) UnnamedOrange. Licensed under the MIT License.
 * See the LICENSE file in the repository root for full license text.
 */

#pragma once

#include <cstdio>

// Lex interface.
extern FILE* yyin;

// YACC interface.
#include "sysy.tab.hpp"
