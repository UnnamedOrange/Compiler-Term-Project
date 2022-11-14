/**
 * @file global_variables.hpp
 * @author UnnamedOrange
 * @brief Global variables.
 *
 * @copyright Copyright (c) UnnamedOrange. Licensed under the MIT License.
 * See the LICENSE file in the repository root for full license text.
 */

#pragma once

#include <filesystem>

namespace compiler::global
{
    /**
     * @brief SysY source file path.
     */
    std::filesystem::path input_file_path;
    /**
     * @brief Output file path.
     */
    std::filesystem::path output_file_path;
}
