/**
 * @file koopa_to_riscv.h
 * @author UnnamedOrange
 * @brief Compile Koopa IR to RISC-V.
 *
 * @copyright Copyright (c) UnnamedOrange. Licensed under the MIT License.
 * See the LICENSE file in the repository root for full license text.
 */

#pragma once

#include <string>

namespace compiler
{
    /**
     * @brief Compile Koopa IR to RISC-V.
     */
    class koopa_to_riscv
    {
    public:
        /**
         * @brief Compile Koopa IR to RISC-V.
         *
         * @param koopa_ir_str Koopa IR in string.
         * @return std::string RISC-V in string.
         */
        std::string compile(const std::string& koopa_ir_str);
    };
} // namespace compiler
