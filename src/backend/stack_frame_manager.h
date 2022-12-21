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

#if defined(COMPILER_LINK_KOOPA)
#include <koopa.h>
#endif

namespace compiler
{
    /**
     * @brief Stack frame manager for backend.
     * Saving offsets of variables.
     */
    class stack_frame_manager
    {
    public:
#if defined(COMPILER_LINK_KOOPA)
        using variable_t = koopa_raw_value_t;
#else
        using variable_t = const void*; // Should be koopa_raw_value_t.
#endif

    private:
        // 低地址额外的空间。用于保存函数参数。
        size_t additional_lower;
        // 高地址额外的空间。用于保存返回地址。
        size_t additional_upper;
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
         * @brief Allocate additional stack space of lower address.
         * Call this for arguments.
         */
        void alloc_lower(size_t size);
        /**
         * @brief Allocate additional stack space of upper address.
         * Call this for return address.
         */
        void alloc_upper(size_t size);
        /**
         * @brief Check whether a variable ID exists in the manager.
         */
        size_t count(variable_t variable_id) const;
        /**
         * @brief Get the offset of a variable.
         */
        int offset(variable_t variable_id) const;
        /**
         * @brief Get the offset of the additional space of lower address.
         * Always returns 0.
         */
        int offset_lower() const;
        /**
         * @brief Get the offset of the additional space of upper address.
         */
        int offset_upper() const;
        /**
         * @brief Get size of the current stack frame.
         */
        size_t size() const;
        /**
         * @brief Get size of the current stack frame (rounded to multiples of
         * 16).
         */
        size_t rounded_size() const;
    };
} // namespace compiler
