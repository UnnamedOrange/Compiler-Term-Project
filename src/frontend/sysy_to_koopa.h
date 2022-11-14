/**
 * @file sysy_to_koopa.h
 * @author UnnamedOrange
 * @brief Compile SysY to Koopa IR.
 *
 * @copyright Copyright (c) UnnamedOrange. Licensed under the MIT License.
 * See the LICENSE file in the repository root for full license text.
 */

#pragma once

#include <filesystem>
#include <string>

namespace compiler
{
    /**
     * @brief Compile SysY to Koopa IR.
     */
    class sysy_to_koopa
    {
    public:
        /**
         * @brief Compile SysY to Koopa IR.
         *
         * @param input_file_path SysY source file path.
         * @return std::string Koopa IR in string.
         */
        std::string compile(const std::filesystem::path& input_file_path);
    };
} // namespace compiler
