/**
 * @file global_variable_manager.h
 * @author UnnamedOrange
 * @brief Global variable manager for backend.
 *
 * @copyright Copyright (c) UnnamedOrange. Licensed under the MIT License.
 * See the LICENSE file in the repository root for full license text.
 */

#pragma once

#include <string>
#include <unordered_map>

#if defined(COMPILER_LINK_KOOPA)
#include <koopa.h>
#endif

namespace compiler
{
    /**
     * @brief Global variable manager for backend.
     * Saving names of global variables.
     */
    class global_variable_manager
    {
    public:
#if defined(COMPILER_LINK_KOOPA)
        using variable_t = koopa_raw_value_t;
#else
        using variable_t = const void*; // Should be koopa_raw_value_t.
#endif

    private:
        // 变量地址到变量名的映射。
        std::unordered_map<variable_t, std::string> variable_to_name;

    public:
        global_variable_manager();

    public:
        /**
         * @brief Clear the manager.
         */
        void clear();
        /**
         * @brief Register a global variable.
         */
        void alloc(variable_t variable_id, const std::string& name);
        /**
         * @brief Check whether a variable ID exists in the manager.
         */
        size_t count(variable_t variable_id) const;
        /**
         * @brief Query the name of a global variable.
         */
        const std::string& at(variable_t variable_id) const;
    };
} // namespace compiler
