/**
 * @file stack_frame_manager.h
 * @author UnnamedOrange
 * @brief Stack frame manager for backend.
 *
 * @copyright Copyright (c) UnnamedOrange. Licensed under the MIT License.
 * See the LICENSE file in the repository root for full license text.
 */

#pragma once

#include <unordered_map>
#include <vector>

namespace compiler
{
    /**
     * @brief Stack frame manager for backend.
     * Saving offsets of variables.
     */
    class stack_frame_manager
    {
    public:
        using variable_t = const void*; // Should be koopa_raw_value_t.

    private:
        // 保存的偏移量。
        std::vector<size_t> offsets;
        // 变量名到偏移量下标的映射。
        std::unordered_map<variable_t, size_t> variable_to_index;

    public:
        stack_frame_manager();

    public:
        /**
         * @brief Clear the manager.
         * Call this when starting to handle a function.
         */
        void clear();
        /**
         * @brief Allocate stack space for a new variable.
         * Call this when scanning instructions in a function.
         */
        void alloc(variable_t variable_id, size_t size);
        /**
         * @brief Get the offset of a variable.
         */
        int offset(variable_t variable_id) const;
    };
} // namespace compiler
