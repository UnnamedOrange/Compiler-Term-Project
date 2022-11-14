/**
 * @file koopa_to_riscv.cpp
 * @author UnnamedOrange
 * @brief Compile Koopa IR to RISC-V.
 *
 * @copyright Copyright (c) UnnamedOrange. Licensed under the MIT License.
 * See the LICENSE file in the repository root for full license text.
 */

#include "koopa_to_riscv.h"

#include <iostream>

#include <fmt/core.h>

#if defined(COMPILER_LINK_KOOPA)
#include <koopa.h>
#endif

using namespace compiler;

#if defined(COMPILER_LINK_KOOPA)
std::string koopa_to_riscv::compile(const std::string& koopa_ir_str)
{
    // TODO: Implement the compiler.
    return "";
}
#else
#pragma message("Koopa to RISC-V is not supported without libkoopa.")
std::string koopa_to_riscv::compile(const std::string&)
{
    std::cerr
        << fmt::format(
               "[Error] Koopa to RISC-V is not supported without libkoopa.")
        << std::endl;
    std::exit(1);
    return "";
}
#endif
