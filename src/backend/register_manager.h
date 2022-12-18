/**
 * @file register_manager.h
 * @author UnnamedOrange
 * @brief Register manager for backend.
 *
 * @copyright Copyright (c) UnnamedOrange. Licensed under the MIT License.
 * See the LICENSE file in the repository root for full license text.
 */

#pragma once

#include <array>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#if defined(COMPILER_LINK_KOOPA)
#include <koopa.h>
#endif

namespace compiler
{
    /**
     * @brief Register manager for backend.
     */
    class register_manager
    {
    public:
#if defined(COMPILER_LINK_KOOPA)
        using variable_t = koopa_raw_value_t;
#else
        using variable_t = const void*; // Should be koopa_raw_value_t.
#endif

    public:
        inline static constexpr auto reg_ret = "a0";
        inline static constexpr auto reg_x = "t1";
        inline static constexpr auto reg_y = "t2";
        inline static constexpr auto reg_z = "t3";
        inline static constexpr std::array reg_names = {
            "t0",
            // "t1", "t2", "t3", // 用于运算结果和操作数。
            "t4",
            "t5",
            "t6",
            // "a0", // 用于返回值。
            "a1",
            "a2",
            "a3",
            "a4",
            "a5",
            "a6",
            "a7",
        };
        std::array<std::vector<variable_t>, reg_names.size()> var_by_reg;
        std::map<variable_t, size_t> reg_by_var;

    private:
        size_t random_vacant_reg() const
        {
            for (size_t i = 0; i < reg_names.size(); i++)
                if (var_by_reg[i].empty())
                    return i;
            throw std::runtime_error("[Error] No vacant register.");
        }

    public:
        std::string get_reg(variable_t x1)
        {
            // 暂时直接分配寄存器，不考虑寄存器不够的情况。
            if (!reg_by_var.count(x1))
            {
                int reg = random_vacant_reg();
                var_by_reg[reg].push_back(x1);
                reg_by_var[x1] = reg;
            }
            return operator[](x1);
        }
        std::string operator[](variable_t var_name) const
        {
            return reg_names[reg_by_var.at(var_name)];
        }
    };
} // namespace compiler
