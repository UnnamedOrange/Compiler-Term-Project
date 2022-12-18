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

#include "register_manager.h"
#include "stack_frame_manager.h"

using namespace compiler;

register_manager rm;
stack_frame_manager sfm;

std::string to_riscv(const std::string&);
std::string visit(const koopa_raw_program_t&);
std::string visit(const koopa_raw_slice_t&);
std::string visit(const koopa_raw_function_t&);
std::string visit(const koopa_raw_basic_block_t&);
std::string visit(const koopa_raw_value_t&);
std::string visit(const koopa_raw_return_t&);
std::string visit(const koopa_raw_binary_t&, const koopa_raw_value_t&);
std::string visit(const koopa_raw_load_t&, const koopa_raw_value_t&);
std::string visit(const koopa_raw_store_t&);

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
    auto ret = fmt::format("{}:\n", func->name + 1);

    // 重置栈帧。
    sfm.clear();
    // 扫描函数中的所有指令, 算出需要分配的栈空间总量。
    {
        auto basic_blocks = func->bbs;
        for (size_t i = 0; i < basic_blocks.len; i++)
        {
            auto basic_block_ptr = reinterpret_cast<koopa_raw_basic_block_t>(
                basic_blocks.buffer[i]);
            auto basic_block = basic_block_ptr->insts;
            for (size_t j = 0; j < basic_block.len; j++)
            {
                auto instruction =
                    reinterpret_cast<koopa_raw_value_t>(basic_block.buffer[j]);
                // 为涉及的变量分配栈空间。
                {
                    if (instruction->ty->tag == KOOPA_RTT_UNIT)
                        continue; // 没有返回值，跳过。
                    // 暂时认为都是 int32_t。
                    sfm.alloc(instruction, 4);
                    // 不用单独考虑操作数，因为操作数一定是算出来的。
                }
            }
        }
    }
    // 计算实际的栈帧大小，并生成导言。
    size_t stack_frame_size =
        (sfm.size() + 15) / 16 * 16; // 向上取整到 16 的倍数。
    if (stack_frame_size <= 2048)    // [-2048, 2047]
        ret += fmt::format("    addi sp, sp, -{}\n", stack_frame_size);
    else // 太大，使用 li 指令代替立即数。
    {
        ret += fmt::format("    li {}, -{}\n", rm.reg_y, stack_frame_size);
        ret += fmt::format("    add sp, sp, {}\n", rm.reg_y);
    }

    // 访问所有基本块。
    ret += visit(func->bbs);
    // 后记在 return 指令处生成。

    return ret;
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
    case KOOPA_RVT_ALLOC:
        // 无需处理 alloc 指令。
        break;
    case KOOPA_RVT_LOAD:
        // 访问 load 指令。
        ret += visit(kind.data.load, value);
        break;
    case KOOPA_RVT_STORE:
        // 访问 store 指令。
        ret += visit(kind.data.store);
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
    std::string reg_ret = rm.reg_ret;

    // 生成保存返回值的指令。
    if (return_inst.value->kind.tag == KOOPA_RVT_INTEGER)
        ret += fmt::format("    li {}, {}\n", reg_ret,
                           return_inst.value->kind.data.integer.value);
    else
    {
        // 将变量加载到寄存器。
        size_t offset = sfm.offset(return_inst.value);
        if (offset < 2048)
            ret += fmt::format("    lw {}, {}(sp)\n", reg_ret, offset);
        else // 太大，使用 li 指令代替立即数。
        {
            ret += fmt::format("    li {}, {}\n", reg_ret, offset);
            ret += fmt::format("    lw {}, {}(sp)\n", reg_ret, reg_ret);
        }
    }

    // 计算实际的栈帧大小，并生成后记。
    size_t stack_frame_size =
        (sfm.size() + 15) / 16 * 16; // 向上取整到 16 的倍数。
    if (stack_frame_size < 2048)     // [-2048, 2047]
        ret += fmt::format("    addi sp, sp, {}\n", stack_frame_size);
    else // 太大，使用 li 指令代替立即数。
    {
        ret += fmt::format("    li {}, {}\n", rm.reg_y, stack_frame_size);
        ret += fmt::format("    add sp, sp, {}\n", rm.reg_y);
    }

    // 生成 ret 指令。
    ret += fmt::format("    ret\n");
    return ret;
}
std::string visit(const koopa_raw_binary_t& binary_inst,
                  const koopa_raw_value_t& parent_value)
{
    std::string ret;
    std::string reg_x = rm.reg_x;
    std::string reg_y = rm.reg_y;
    std::string reg_z = rm.reg_z;

    if (binary_inst.lhs->kind.tag == KOOPA_RVT_INTEGER)
    {
        // 将字面量存入寄存器。
        int value = binary_inst.lhs->kind.data.integer.value;
        if (value)
            ret += fmt::format("    li {}, {}\n", reg_y, value);
        else
            reg_y = "x0";
    }
    else
    {
        // 将变量加载到寄存器。
        size_t offset = sfm.offset(binary_inst.lhs);
        if (offset < 2048)
            ret += fmt::format("    lw {}, {}(sp)\n", reg_y, offset);
        else // 太大，使用 li 指令代替立即数。
        {
            ret += fmt::format("    li {}, {}\n", reg_y, offset);
            ret += fmt::format("    lw {}, {}(sp)\n", reg_y, reg_y);
        }
    }

    if (binary_inst.rhs->kind.tag == KOOPA_RVT_INTEGER)
    {
        // 将字面量存入寄存器。
        int value = binary_inst.rhs->kind.data.integer.value;
        if (value)
            ret += fmt::format("    li {}, {}\n", reg_z, value);
        else
            reg_z = "x0";
    }
    else
    {
        // 将变量加载到寄存器。
        size_t offset = sfm.offset(binary_inst.rhs);
        if (offset < 2048)
            ret += fmt::format("    lw {}, {}(sp)\n", reg_z, offset);
        else // 太大，使用 li 指令代替立即数。
        {
            ret += fmt::format("    li {}, {}\n", reg_z, offset);
            ret += fmt::format("    lw {}, {}(sp)\n", reg_z, reg_z);
        }
    }

    switch (binary_inst.op)
    {
    case KOOPA_RBO_ADD:
    {
        ret += fmt::format("    add {}, {}, {}\n", reg_x, reg_y, reg_z);
        break;
    }
    case KOOPA_RBO_SUB:
    {
        ret += fmt::format("    sub {}, {}, {}\n", reg_x, reg_y, reg_z);
        break;
    }
    case KOOPA_RBO_MUL:
    {
        ret += fmt::format("    mul {}, {}, {}\n", reg_x, reg_y, reg_z);
        break;
    }
    case KOOPA_RBO_DIV:
    {
        ret += fmt::format("    div {}, {}, {}\n", reg_x, reg_y, reg_z);
        break;
    }
    case KOOPA_RBO_MOD:
    {
        ret += fmt::format("    rem {}, {}, {}\n", reg_x, reg_y, reg_z);
        break;
    }
    case KOOPA_RBO_LT:
    {
        ret += fmt::format("    slt {}, {}, {}\n", reg_x, reg_y, reg_z);
        break;
    }
    case KOOPA_RBO_GT:
    {
        ret += fmt::format("    sgt {}, {}, {}\n", reg_x, reg_y, reg_z);
        break;
    }
    case KOOPA_RBO_LE:
    {
        ret += fmt::format("    sgt {}, {}, {}\n", reg_x, reg_y, reg_z);
        ret += fmt::format("    seqz {}, {}\n", reg_x, reg_x);
        break;
    }
    case KOOPA_RBO_GE:
    {
        ret += fmt::format("    slt {}, {}, {}\n", reg_x, reg_y, reg_z);
        ret += fmt::format("    seqz {}, {}\n", reg_x, reg_x);
        break;
    }
    case KOOPA_RBO_EQ:
    {
        ret += fmt::format("    xor {}, {}, {}\n", reg_x, reg_y, reg_z);
        ret += fmt::format("    seqz {}, {}\n", reg_x, reg_x);
        break;
    }
    case KOOPA_RBO_NOT_EQ:
    {
        ret += fmt::format("    xor {}, {}, {}\n", reg_x, reg_y, reg_z);
        ret += fmt::format("    snez {}, {}\n", reg_x, reg_x);
        break;
    }
    case KOOPA_RBO_AND:
    {
        ret += fmt::format("    and {}, {}, {}\n", reg_x, reg_y, reg_z);
        break;
    }
    case KOOPA_RBO_OR:
    {
        ret += fmt::format("    or {}, {}, {}\n", reg_x, reg_y, reg_z);
        break;
    }
    case KOOPA_RBO_XOR:
    {
        ret += fmt::format("    xor {}, {}, {}\n", reg_x, reg_y, reg_z);
        break;
    }
    default:
        // 其他类型暂时遇不到。
        assert(false);
    }

    // 将结果保存至内存。
    {
        size_t offset = sfm.offset(parent_value);
        if (offset < 2048)
            ret += fmt::format("    sw {}, {}(sp)\n", reg_x, offset);
        else // 太大，使用 li 指令代替立即数。
        {
            ret += fmt::format("    li {}, {}\n", reg_y, offset);
            ret += fmt::format("    sw {}, {}(sp)\n", reg_x, reg_y);
        }
    }
    return ret;
}
std::string visit(const koopa_raw_load_t& load_inst,
                  const koopa_raw_value_t& parent_value)
{
    std::string ret;
    std::string reg_x = rm.reg_x; // 存储值的寄存器。
    std::string reg_y = rm.reg_y; // 保存偏移量的寄存器。

    // 将变量加载到寄存器。
    {
        size_t offset = sfm.offset(load_inst.src);
        if (offset < 2048)
            ret += fmt::format("    lw {}, {}(sp)\n", reg_x, offset);
        else // 太大，使用 li 指令代替立即数。
        {
            ret += fmt::format("    li {}, {}\n", reg_y, offset);
            ret += fmt::format("    lw {}, {}(sp)\n", reg_x, reg_y);
        }
    }

    // 将结果保存至内存。
    {
        size_t offset = sfm.offset(parent_value);
        if (offset < 2048)
            ret += fmt::format("    sw {}, {}(sp)\n", reg_x, offset);
        else // 太大，使用 li 指令代替立即数。
        {
            ret += fmt::format("    li {}, {}\n", reg_y, offset);
            ret += fmt::format("    sw {}, {}(sp)\n", reg_x, reg_y);
        }
    }
    return ret;
}
std::string visit(const koopa_raw_store_t& store_inst)
{
    std::string ret;
    std::string reg_x = rm.reg_x; // 存储值的寄存器。
    std::string reg_y = rm.reg_y; // 保存偏移量的寄存器。

    // 将值加载到寄存器。
    {
        // 如果是常量，将整数写入寄存器。
        if (store_inst.value->kind.tag == KOOPA_RVT_INTEGER)
            ret += fmt::format("    li {}, {}\n", reg_x,
                               store_inst.value->kind.data.integer.value);
        else // 将变量加载到寄存器。
        {
            size_t offset = sfm.offset(store_inst.value);
            if (offset < 2048)
                ret += fmt::format("    lw {}, {}(sp)\n", reg_x, offset);
            else // 太大，使用 li 指令代替立即数。
            {
                ret += fmt::format("    li {}, {}\n", reg_y, offset);
                ret += fmt::format("    lw {}, {}(sp)\n", reg_x, reg_y);
            }
        }
    }

    // 将结果保存至内存。
    {
        size_t offset = sfm.offset(store_inst.dest);
        if (offset < 2048)
            ret += fmt::format("    sw {}, {}(sp)\n", reg_x, offset);
        else // 太大，使用 li 指令代替立即数。
        {
            ret += fmt::format("    li {}, {}\n", reg_y, offset);
            ret += fmt::format("    sw {}, {}(sp)\n", reg_x, reg_y);
        }
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
