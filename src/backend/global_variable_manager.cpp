/**
 * @file global_variable_manager.cpp
 * @author UnnamedOrange
 * @brief Global variable manager for backend.
 *
 * @copyright Copyright (c) UnnamedOrange. Licensed under the MIT License.
 * See the LICENSE file in the repository root for full license text.
 */

#include "global_variable_manager.h"

using namespace compiler;

global_variable_manager::global_variable_manager() { clear(); }

void global_variable_manager::clear() { variable_to_name.clear(); }
void global_variable_manager::alloc(variable_t variable_id,
                                    const std::string& name)
{
    variable_to_name[variable_id] = name;
}
size_t global_variable_manager::count(variable_t variable_id) const
{
    return variable_to_name.count(variable_id);
}
const std::string& global_variable_manager::at(variable_t variable_id) const
{
    return variable_to_name.at(variable_id);
}
