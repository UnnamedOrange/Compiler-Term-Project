/**
 * @file koopa_to_riscv.cpp
 * @author UnnamedOrange
 * @brief Compile Koopa IR to RISC-V.
 *
 * @copyright Copyright (c) UnnamedOrange. Licensed under the MIT License.
 * See the LICENSE file in the repository root for full license text.
 */

#include "koopa_to_riscv.h"

#include <array>
#include <cassert>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

#include <fmt/core.h>

#if defined(COMPILER_LINK_KOOPA)
#include <koopa.h>
#endif

using namespace compiler;

#if defined(COMPILER_LINK_KOOPA)

class register_manager
{
private:
    inline static constexpr std::array reg_names = {
        "t0", "t1", "t2", "t3", "t4", "t5", "t6",
        // "a0", // 用于返回值。
        "a1", "a2", "a3", "a4", "a5",
        // "a6", "a7", // 用于立即数。
    };
    std::array<std::vector<koopa_raw_value_t>, reg_names.size()> var_by_reg;
    std::map<koopa_raw_value_t, size_t> reg_by_var;

private:
    size_t random_vacant_reg() const
    {
        for (size_t i = 0; i < reg_names.size(); i++)
            if (var_by_reg[i].empty())
                return i;
        throw std::runtime_error("[Error] No vacant register.");
    }

public:
    std::string get_reg(koopa_raw_value_t x1)
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
    std::string operator[](koopa_raw_value_t var_name) const
    {
        return reg_names[reg_by_var.at(var_name)];
    }
};
register_manager rm;

std::string to_riscv(const std::string&);
std::string visit(const koopa_raw_program_t&);
std::string visit(const koopa_raw_slice_t&);
std::string visit(const koopa_raw_function_t&);
std::string visit(const koopa_raw_basic_block_t&);
std::string visit(const koopa_raw_value_t&);
std::string visit(const koopa_raw_return_t&);
std::string visit(const koopa_raw_binary_t&, const koopa_raw_value_t&);

std::string to_riscv(const std::string& koopa)
{
    koopa_program_t program;
    koopa_error_code_t ret = koopa_parse_from_string(koopa.c_str(), &program);
    assert(ret == KOOPA_EC_SUCCESS);
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    koopa_delete_program(program);
    auto ret_riscv = visit(raw);
    koopa_delete_raw_program_builder(builder);
    return ret_riscv;
}
std::string visit(const koopa_raw_program_t& program)
{
    std::string ret;
    // 访问所有全局变量。
    ret += "    .text\n";
    ret += visit(program.values);
    for (size_t i = 0; i < program.funcs.len; ++i)
    {
        auto entry =
            reinterpret_cast<koopa_raw_function_t>(program.funcs.buffer[i]);
        ret += fmt::format("    .globl {}\n", entry->name + 1);
    }
    // 访问所有函数。
    ret += visit(program.funcs);
    return ret;
}
std::string visit(const koopa_raw_slice_t& slice)
{
    std::string ret;
    for (size_t i = 0; i < slice.len; ++i)
    {
        auto ptr = slice.buffer[i];
        // 根据 slice 的 kind 决定将 ptr 视作何种元素。
        switch (slice.kind)
        {
        case KOOPA_RSIK_FUNCTION:
            // 访问函数。
            ret += visit(reinterpret_cast<koopa_raw_function_t>(ptr));
            break;
        case KOOPA_RSIK_BASIC_BLOCK:
            // 访问基本块。
            ret += visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
            break;
        case KOOPA_RSIK_VALUE:
            // 访问指令。
            ret += visit(reinterpret_cast<koopa_raw_value_t>(ptr));
            break;
        default:
            // 我们暂时不会遇到其他内容, 于是不对其做任何处理。
            assert(false);
        }
    }
    return ret;
}
std::string visit(const koopa_raw_function_t& func)
{
    // 执行一些其他的必要操作。
    // ...
    // 访问所有基本块。
    return fmt::format("{}:\n{}", func->name + 1, visit(func->bbs));
}
std::string visit(const koopa_raw_basic_block_t& bb)
{
    // 执行一些其他的必要操作。
    // ...
    // 访问所有指令。
    return visit(bb->insts);
}
std::string visit(const koopa_raw_value_t& value)
{
    std::string ret;
    // 根据指令类型判断后续需要如何访问。
    const auto& kind = value->kind;
    switch (kind.tag)
    {
    case KOOPA_RVT_RETURN:
        // 访问 return 指令。
        ret += visit(kind.data.ret);
        break;
    case KOOPA_RVT_BINARY:
        // 访问 binary 指令。
        ret += visit(kind.data.binary, value);
        break;
    case KOOPA_RVT_INTEGER:
        // 访问 integer 指令。
        // ret += visit(kind.data.integer);
        break;
    default:
        // 其他类型暂时遇不到。
        assert(false);
    }
    return ret;
}
std::string visit(const koopa_raw_return_t& return_inst)
{
    std::string ret;
    if (return_inst.value->kind.tag == KOOPA_RVT_INTEGER)
        ret += fmt::format("    li a0, {}\n    ret\n",
                           return_inst.value->kind.data.integer.value);
    else
    {
        std::string reg_y = rm.get_reg(return_inst.value);
        ret += fmt::format("    mv a0, {}\n    ret\n", reg_y);
    }
    return ret;
}
std::string visit(const koopa_raw_binary_t& binary_inst,
                  const koopa_raw_value_t& parent_value)
{
    std::string ret;
    std::string reg_l = "a6";
    std::string reg_r = "a7";
    if (binary_inst.lhs->kind.tag == KOOPA_RVT_INTEGER)
    {
        int value = binary_inst.lhs->kind.data.integer.value;
        if (value)
            ret += fmt::format("    li {}, {}\n", reg_l, value);
        else
            reg_l = "x0";
    }
    else
        reg_l = rm.get_reg(binary_inst.lhs);
    if (binary_inst.rhs->kind.tag == KOOPA_RVT_INTEGER)
    {
        int value = binary_inst.rhs->kind.data.integer.value;
        if (value)
            ret += fmt::format("    li {}, {}\n", reg_r, value);
        else
            reg_r = "x0";
    }
    else
        reg_r = rm.get_reg(binary_inst.rhs);
    std::string reg_x = rm.get_reg(parent_value);

    switch (binary_inst.op)
    {
    case KOOPA_RBO_ADD:
    {
        ret += fmt::format("    add {}, {}, {}\n", reg_x, reg_l, reg_r);
        break;
    }
    case KOOPA_RBO_SUB:
    {
        ret += fmt::format("    sub {}, {}, {}\n", reg_x, reg_l, reg_r);
        break;
    }
    case KOOPA_RBO_MUL:
    {
        ret += fmt::format("    mul {}, {}, {}\n", reg_x, reg_l, reg_r);
        break;
    }
    case KOOPA_RBO_DIV:
    {
        ret += fmt::format("    div {}, {}, {}\n", reg_x, reg_l, reg_r);
        break;
    }
    case KOOPA_RBO_MOD:
    {
        ret += fmt::format("    rem {}, {}, {}\n", reg_x, reg_l, reg_r);
        break;
    }
    case KOOPA_RBO_LT:
    {
        ret += fmt::format("    slt {}, {}, {}\n", reg_x, reg_l, reg_r);
        break;
    }
    case KOOPA_RBO_GT:
    {
        ret += fmt::format("    sgt {}, {}, {}\n", reg_x, reg_l, reg_r);
        break;
    }
    case KOOPA_RBO_LE:
    {
        ret += fmt::format("    sgt {}, {}, {}\n", reg_x, reg_l, reg_r);
        ret += fmt::format("    seqz {}, {}\n", reg_x, reg_x);
        break;
    }
    case KOOPA_RBO_GE:
    {
        ret += fmt::format("    slt {}, {}, {}\n", reg_x, reg_l, reg_r);
        ret += fmt::format("    seqz {}, {}\n", reg_x, reg_x);
        break;
    }
    case KOOPA_RBO_EQ:
    {
        ret += fmt::format("    xor {}, {}, {}\n", reg_x, reg_l, reg_r);
        ret += fmt::format("    seqz {}, {}\n", reg_x, reg_x);
        break;
    }
    case KOOPA_RBO_NOT_EQ:
    {
        ret += fmt::format("    xor {}, {}, {}\n", reg_x, reg_l, reg_r);
        ret += fmt::format("    snez {}, {}\n", reg_x, reg_x);
        break;
    }
    case KOOPA_RBO_AND:
    {
        ret += fmt::format("    and {}, {}, {}\n", reg_x, reg_l, reg_r);
        break;
    }
    case KOOPA_RBO_OR:
    {
        ret += fmt::format("    or {}, {}, {}\n", reg_x, reg_l, reg_r);
        break;
    }
    case KOOPA_RBO_XOR:
    {
        ret += fmt::format("    xor {}, {}, {}\n", reg_x, reg_l, reg_r);
        break;
    }
    default:
        // 其他类型暂时遇不到。
        assert(false);
    }
    return ret;
}

std::string koopa_to_riscv::compile(const std::string& koopa_ir_str)
{
    return to_riscv(koopa_ir_str);
}

#else
#pragma message("Koopa to RISC-V is not supported without libkoopa.")
std::string koopa_to_riscv::compile(const std::string&)
{
    std::cerr
        << fmt::format(
               "[Error] Koopa to RISC-V is not supported without libkoopa.")
        << std::endl;
    std::exit(1);
    return "";
}
#endif
