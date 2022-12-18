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
}
void stack_frame_manager::alloc(variable_t variable_id, size_t size)
{
    if (variable_to_index.count(variable_id))
        return; // 不要重复分配。
    variable_to_index[variable_id] = offsets.size();
    offsets.push_back(offsets.back() + size);
}
int stack_frame_manager::offset(variable_t variable_id) const
{
    size_t index = variable_to_index.at(variable_id);
    return offsets[index];
}
