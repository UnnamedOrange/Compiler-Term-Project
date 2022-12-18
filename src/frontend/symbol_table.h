/**
 * @file symbol_table.h
 * @author UnnamedOrange
 * @brief Symbol table for frontend.
 *
 * @copyright Copyright (c) UnnamedOrange. Licensed under the MIT License.
 * See the LICENSE file in the repository root for full license text.
 */

#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace compiler
{
    struct symbol_base_t
    {
        std::string internal_name;
    };

    struct symbol_const_t : public symbol_base_t
    {
        int value;
    };

    struct symbol_variable_t : public symbol_base_t
    {
        // TODO: Design member variables.
    };

    using symbol_t = std::variant<symbol_const_t, symbol_variable_t>;

    /**
     * @brief Symbol table for frontend.
     */
    class symbol_table_t
    {
    public:
        using table_t = std::unordered_map<std::string, symbol_t>;

    private:
        std::vector<table_t> table_stack;

    public:
        symbol_table_t();

    public:
        /**
         * @brief Push a table into the table stack.
         */
        void push();
        /**
         * @brief Pop a table from the table stack.
         */
        void pop();

    public:
        /**
         * @brief Insert a symbol into the top table.
         */
        void insert(const std::string& raw_name, symbol_t symbol);
        /**
         * @brief Check whether a symbol exists according to its raw name.
         */
        size_t count(const std::string& raw_name) const;
        /**
         * @brief Query a symbol according to its raw name.
         */
        std::optional<symbol_t> at(const std::string& raw_name) const;
    };
} // namespace compiler
