/**
 * @file symbol_table.cpp
 * @author UnnamedOrange
 * @brief Symbol table for frontend.
 *
 * @copyright Copyright (c) UnnamedOrange. Licensed under the MIT License.
 * See the LICENSE file in the repository root for full license text.
 */

#include <type_traits>
#include <variant>

#include "symbol_table.h"

#include <fmt/core.h>

using namespace compiler;

symbol_table_t::symbol_table_t() : table_stack(1) {}

void symbol_table_t::push() { table_stack.emplace_back(); }
void symbol_table_t::pop() { table_stack.pop_back(); }

void symbol_table_t::insert(const std::string& raw_name, symbol_t symbol)
{
    std::visit(
        [&](auto& symbol) {
            using T = std::decay_t<decltype(symbol)>;
            if constexpr (std::is_same_v<T, symbol_function_t>)
            {
                symbol.internal_name = raw_name;
            }
            else
            {
                auto original_internal_name =
                    fmt::format("{}_{}", raw_name, table_stack.size());
                symbol.internal_name =
                    fmt::format("{}_{}", original_internal_name,
                                ++use_count[original_internal_name]);
            }
        },
        symbol);
    table_stack.back()[raw_name] = symbol;
}
size_t symbol_table_t::count(const std::string& raw_name) const
{
    size_t ret{};
    for (const auto& table : table_stack)
        ret += table.count(raw_name);
    return ret;
}
std::optional<symbol_t> symbol_table_t::at(const std::string& raw_name) const
{
    for (auto it = table_stack.crbegin(); it != table_stack.crend(); it++)
        if ((*it).count(raw_name))
            return (*it).at(raw_name);
    return std::nullopt;
}
bool symbol_table_t::is_global(const std::string& raw_name) const
{
    for (size_t i = table_stack.size() - 1; ~i; i--)
    {
        if (table_stack[i].count(raw_name))
        {
            if (i)
                return false;
            else
                return true;
        }
    }
    return false;
}
