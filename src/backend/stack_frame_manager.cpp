/**
 * @file stack_frame_manager.cpp
 * @author UnnamedOrange
 * @brief Stack frame manager for backend.
 *
 * @copyright Copyright (c) UnnamedOrange. Licensed under the MIT License.
 * See the LICENSE file in the repository root for full license text.
 */

#include "stack_frame_manager.h"

using namespace compiler;

stack_frame_manager::stack_frame_manager() { clear(); }

void stack_frame_manager::clear()
{
    offsets.clear();
    variable_to_index.clear();
    offsets.push_back(0);
    additional_lower = 0;
    additional_upper = 0;
}
void stack_frame_manager::alloc(variable_t variable_id, size_t size)
{
    if (variable_to_index.count(variable_id))
        return; // 不要重复分配。
    variable_to_index[variable_id] = offsets.size() - 1;
    offsets.push_back(offsets.back() + size);
}
void stack_frame_manager::alloc_lower(size_t size) { additional_lower = size; }
void stack_frame_manager::alloc_upper(size_t size) { additional_upper = size; }
size_t stack_frame_manager::count(variable_t variable_id) const
{
    return variable_to_index.count(variable_id);
}
int stack_frame_manager::offset(variable_t variable_id) const
{
    size_t index = variable_to_index.at(variable_id);
    return additional_lower + offsets[index];
}
int stack_frame_manager::offset_lower() const { return 0; }
int stack_frame_manager::offset_upper() const
{
    return additional_lower + offsets.back();
}
size_t stack_frame_manager::size() const
{
    return additional_lower + offsets.back() + additional_upper;
}
size_t stack_frame_manager::rounded_size() const
{
    constexpr size_t rounded_to = 16;
    return (size() + rounded_to - 1) / rounded_to * rounded_to;
}
