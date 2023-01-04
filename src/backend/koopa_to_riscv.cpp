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
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

#include <fmt/core.h>

#if defined(COMPILER_LINK_KOOPA)

#include <koopa.h>

#include "global_variable_manager.h"
#include "register_manager.h"
#include "stack_frame_manager.h"

using namespace compiler;

register_manager rm;
stack_frame_manager sfm;
global_variable_manager gvm;

koopa_raw_function_t current_function;

/**
 * @brief Generate codes that load a value in stack to a register.
 */
std::string generate_load(const std::string& target_reg,
                          const std::string& temp_reg, int offset)
{
    std::string ret;
    if (-2048 <= offset && offset < 2048)
        ret += fmt::format("    lw {}, {}(sp)\n", target_reg, offset);
    else // 太大，使用 li 指令代替立即数。
    {
        ret += fmt::format("    li {}, {}\n", temp_reg, offset);
        ret += fmt::format("    add sp, sp, {}\n", temp_reg);
        ret += fmt::format("    lw {}, 0(sp)\n", target_reg);
        ret += fmt::format("    sub sp, sp, {}\n", temp_reg);
    }
    return ret;
}
/**
 * @brief Generate codes that load a value to a register.
 */
std::string generate_load(const std::string& target_reg,
                          const std::string& temp_reg,
                          const koopa_raw_value_t& value)
{
    std::string ret;
    if (gvm.count(value))
    {
        ret += fmt::format("    la {}, {}\n", target_reg, gvm.at(value));
        ret += fmt::format("    lw {}, 0({})\n", target_reg, target_reg);
    }
    else
    {
        auto offset = sfm.offset(value);
        ret += generate_load(target_reg, temp_reg, offset);
    }
    return ret;
}
/**
 * @brief Generate codes that store the value in a register into stack.
 */
std::string generate_store(const std::string& target_reg,
                           const std::string& temp_reg, int offset)
{
    std::string ret;
    if (-2048 <= offset && offset < 2048)
        ret += fmt::format("    sw {}, {}(sp)\n", target_reg, offset);
    else // 太大，使用 li 指令代替立即数。
    {
        ret += fmt::format("    li {}, {}\n", temp_reg, offset);
        ret += fmt::format("    add sp, sp, {}\n", temp_reg);
        ret += fmt::format("    sw {}, 0(sp)\n", target_reg);
        ret += fmt::format("    sub sp, sp, {}\n", temp_reg);
    }
    return ret;
}
/**
 * @brief Generate codes that store the value in a register.
 */
std::string generate_store(const std::string& target_reg,
                           const std::string& temp_reg,
                           const koopa_raw_value_t& value)
{
    std::string ret;
    if (gvm.count(value))
    {
        ret += fmt::format("    la {}, {}\n", temp_reg, gvm.at(value));
        ret += fmt::format("    sw {}, 0({})\n", target_reg, temp_reg);
    }
    else
    {
        auto offset = sfm.offset(value);
        ret += generate_store(target_reg, temp_reg, offset);
    }
    return ret;
}

size_t get_size(const koopa_raw_type_t& value)
{
    switch (value->tag)
    {
    case KOOPA_RTT_INT32:
        return 4;
    case KOOPA_RTT_UNIT:
        return 0;
    case KOOPA_RTT_ARRAY:
    {
        const auto& a = value->data.array;
        return a.len * get_size(a.base);
    }
    case KOOPA_RTT_POINTER:
        return 4;
    case KOOPA_RTT_FUNCTION:
        return 0;
    }
    return 0;
}

std::string generate_global_init(const koopa_raw_value_t& init)
{
    std::string ret;
    switch (init->kind.tag)
    {
    case KOOPA_RVT_ZERO_INIT:
    {
        ret += fmt::format("    .zero {}\n", get_size(init->ty));
        break;
    }
    case KOOPA_RVT_INTEGER:
    {
        ret += fmt::format("    .word {}\n", init->kind.data.integer.value);
        break;
    }
    case KOOPA_RVT_AGGREGATE:
    {
        const auto& aggregate = init->kind.data.aggregate;
        const auto& slice = aggregate.elems;
        for (uint32_t i = 0; i < slice.len; ++i)
        {
            auto ptr = slice.buffer[i];
            ret +=
                generate_global_init(reinterpret_cast<koopa_raw_value_t>(ptr));
        }
        break;
    }
    default:
        break;
    }
    return ret;
}

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
std::string visit(const koopa_raw_jump_t&);
std::string visit(const koopa_raw_branch_t&);
std::string visit(const koopa_raw_call_t&, const koopa_raw_value_t&);
std::string visit(const koopa_raw_global_alloc_t&, const koopa_raw_value_t&);
std::string visit(const koopa_raw_get_elem_ptr_t&, const koopa_raw_value_t&);
std::string visit(const koopa_raw_get_ptr_t&, const koopa_raw_value_t&);
std::string visit_array_or_pointer(const koopa_raw_value_t&,
                                   const koopa_raw_value_t&,
                                   const koopa_raw_value_t&);

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
    ret += visit(program.values);
    // 访问所有函数。
    ret += visit(program.funcs);
    return ret;
}
std::string visit(const koopa_raw_slice_t& slice)
{
    std::string ret;
    for (uint32_t i = 0; i < slice.len; ++i)
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
    // 如果是声明，则跳过。
    if (!func->bbs.len)
        return "";

    current_function = func;

    std::string ret;

    ret += "    .text\n";
    ret += fmt::format("    .globl {}\n", func->name + 1);
    ret += fmt::format("{}:\n", func->name + 1);

    // 重置栈帧。
    sfm.clear();
    // 扫描函数中的所有指令, 算出需要分配的栈空间总量。
    {
        sfm.alloc_upper(4); // 为了方便，总是保存返回地址。

        uint32_t max_parameter_count = 0;

        auto basic_blocks = func->bbs;
        for (uint32_t i = 0; i < basic_blocks.len; i++)
        {
            auto basic_block_ptr = reinterpret_cast<koopa_raw_basic_block_t>(
                basic_blocks.buffer[i]);
            auto basic_block = basic_block_ptr->insts;
            for (uint32_t j = 0; j < basic_block.len; j++)
            {
                auto instruction =
                    reinterpret_cast<koopa_raw_value_t>(basic_block.buffer[j]);
                // 为涉及的变量、参数、返回地址分配栈空间。
                {
                    if (instruction->kind.tag == KOOPA_RVT_CALL)
                    {
                        max_parameter_count =
                            std::max(max_parameter_count,
                                     instruction->kind.data.call.args.len);
                    }

                    if (instruction->ty->tag != KOOPA_RTT_UNIT)
                    {
                        sfm.alloc(instruction, get_size(instruction->ty));
                        // 不用单独考虑操作数，因为操作数一定是算出来的。
                    }
                }
            }
        }
        if (max_parameter_count > 8)
            sfm.alloc_lower((max_parameter_count - 8) * 4);
    }
    // 计算实际的栈帧大小，并生成导言。
    {
        size_t stack_frame_size = sfm.rounded_size();
        if (stack_frame_size <= 2048) // [-2048, 2047]
            ret += fmt::format("    addi sp, sp, -{}\n", stack_frame_size);
        else // 太大，使用 li 指令代替立即数。
        {
            ret += fmt::format("    li {}, -{}\n", rm.reg_y, stack_frame_size);
            ret += fmt::format("    add sp, sp, {}\n", rm.reg_y);
        }
    }

    // 保存 ra 寄存器的值。
    ret += generate_store(rm.reg_ra, rm.reg_x, sfm.offset_upper());

    // 访问所有基本块。
    ret += visit(func->bbs);
    // 后记在 return 指令处生成。

    ret += "\n";

    return ret;
}
std::string visit(const koopa_raw_basic_block_t& bb)
{
    std::string ret;

    // 为基本块增加标签。
    ret += fmt::format("{}:\n", bb->name + 1);
    // 访问所有指令。
    ret += visit(bb->insts);

    return ret;
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
    case KOOPA_RVT_JUMP:
        // 访问 jump 指令。
        ret += visit(kind.data.jump);
        break;
    case KOOPA_RVT_BRANCH:
        // 访问 br 指令。
        ret += visit(kind.data.branch);
        break;
    case KOOPA_RVT_CALL:
        // 访问 call 指令。
        ret += visit(kind.data.call, value);
        break;
    case KOOPA_RVT_GLOBAL_ALLOC:
        // 访问 global 指令。
        ret += visit(kind.data.global_alloc, value);
        break;
    case KOOPA_RVT_GET_ELEM_PTR:
        // 访问 getelemptr 指令。
        ret += visit(kind.data.get_elem_ptr, value);
        break;
    case KOOPA_RVT_GET_PTR:
        // 访问 getelemptr 指令。
        ret += visit(kind.data.get_ptr, value);
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

    // 如果有返回值，则将返回值写入寄存器。
    if (return_inst.value)
    {
        // 生成保存返回值的指令。
        if (return_inst.value->kind.tag == KOOPA_RVT_INTEGER)
            ret += fmt::format("    li {}, {}\n", reg_ret,
                               return_inst.value->kind.data.integer.value);
        else // 将变量加载到寄存器。
            ret += generate_load(reg_ret, rm.reg_x, return_inst.value);
    }
    // 否则直接生成后记。

    // 恢复返回地址。
    ret += generate_load(rm.reg_ra, rm.reg_x, sfm.offset_upper());

    // 计算实际的栈帧大小。
    {
        size_t stack_frame_size = sfm.rounded_size();
        if (stack_frame_size < 2048) // [-2048, 2047]
            ret += fmt::format("    addi sp, sp, {}\n", stack_frame_size);
        else // 太大，使用 li 指令代替立即数。
        {
            ret += fmt::format("    li {}, {}\n", rm.reg_y, stack_frame_size);
            ret += fmt::format("    add sp, sp, {}\n", rm.reg_y);
        }
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
        ret += fmt::format("    li {}, {}\n", reg_y, value);
    }
    else // 将变量加载到寄存器。
        ret += generate_load(reg_y, reg_x, binary_inst.lhs);

    if (binary_inst.rhs->kind.tag == KOOPA_RVT_INTEGER)
    {
        // 将字面量存入寄存器。
        int value = binary_inst.rhs->kind.data.integer.value;
        ret += fmt::format("    li {}, {}\n", reg_z, value);
    }
    else // 将变量加载到寄存器。
        ret += generate_load(reg_z, reg_x, binary_inst.rhs);

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
    ret += generate_store(reg_x, reg_y, parent_value);

    return ret;
}
std::string visit(const koopa_raw_load_t& load_inst,
                  const koopa_raw_value_t& parent_value)
{
    std::string ret;
    std::string reg_x = rm.reg_x; // 保存值的寄存器。
    std::string reg_y = rm.reg_y; // 保存偏移量的寄存器。

    // 将变量加载到寄存器。
    ret += generate_load(reg_x, reg_y, load_inst.src);

    // 将结果保存至内存。
    ret += generate_store(reg_x, reg_y, parent_value);

    return ret;
}
std::string visit(const koopa_raw_store_t& store_inst)
{
    std::string ret;
    std::string reg_x = rm.reg_x; // 保存值的寄存器。
    std::string reg_y = rm.reg_y; // 保存偏移量的寄存器。

    // 将值加载到寄存器。
    {
        // 如果是常量，将整数写入寄存器。
        if (store_inst.value->kind.tag == KOOPA_RVT_INTEGER)
            ret += fmt::format("    li {}, {}\n", reg_x,
                               store_inst.value->kind.data.integer.value);
        else // 将变量加载到寄存器。
        {
            int offset;
            std::string argument_register;
            if (!sfm.count(store_inst.value))
            {
                // 如果栈帧中没有这个变量，说明是第一次读取参数。计算出参数的偏移量。
                uint32_t argument_index{};
                for (; argument_index < current_function->params.len;
                     argument_index++)
                {
                    if (store_inst.value ==
                        current_function->params.buffer[argument_index])
                        break;
                }
                assert(argument_index != current_function->params.len);

                // 根据参数的序号计算出寄存器或偏移量。
                if (argument_index < 8)
                    argument_register = fmt::format("a{}", argument_index);
                else
                    offset = sfm.rounded_size() + 4 * (argument_index - 8);
            }
            else // 否则照常从栈帧中直接找到变量。
                offset = sfm.offset(store_inst.value);

            if (argument_register.empty()) // 不对应寄存器，从内存中加载参数。
                ret += generate_load(reg_x, reg_y, offset);
            else // 将参数从寄存器保存到内存中。
                ret += fmt::format("    mv {}, {}\n", reg_x, argument_register);
        }
    }

    // 将结果保存至内存。
    ret += generate_store(reg_x, reg_y, store_inst.dest);

    return ret;
}
std::string visit(const koopa_raw_jump_t& jump_inst)
{
    return fmt::format("    j {}\n", jump_inst.target->name + 1);
}
std::string visit(const koopa_raw_branch_t& branch_inst)
{
    std::string ret;

    if (branch_inst.cond->kind.tag == KOOPA_RVT_INTEGER)
    {
        // 直接无条件跳转。
        int value = branch_inst.cond->kind.data.integer.value;
        if (value)
            ret += fmt::format("    j {}\n", branch_inst.true_bb->name + 1);
        else
            ret += fmt::format("    j {}\n", branch_inst.false_bb->name + 1);
    }
    else
    {
        // 将变量加载到寄存器。
        std::string reg_x = rm.reg_x; // 保存值的寄存器。
        std::string reg_y = rm.reg_y; // 保存偏移量的寄存器。
        ret += generate_load(reg_x, reg_y, branch_inst.cond);
        ret += fmt::format("    bnez {}, {}\n", reg_x,
                           branch_inst.true_bb->name + 1);
        ret += fmt::format("    j {}\n", branch_inst.false_bb->name + 1);
    }

    return ret;
}
std::string visit(const koopa_raw_call_t& call_inst,
                  const koopa_raw_value_t& parent_value)
{
    std::string ret;

    // 将序号小于等于 8 的参数放入寄存器中。
    for (uint32_t i = 0; i < std::min(8u, call_inst.args.len); i++)
    {
        auto argument =
            reinterpret_cast<koopa_raw_value_t>(call_inst.args.buffer[i]);

        std::string reg_x = fmt::format("a{}", i); // 保存值的寄存器。
        std::string reg_y = rm.reg_y; // 保存偏移量的寄存器。

        // 将参数写入寄存器。
        if (argument->kind.tag == KOOPA_RVT_INTEGER)
        {
            // 将立即数写入寄存器。
            ret += fmt::format("    li {}, {}\n", reg_x,
                               argument->kind.data.integer.value);
        }
        else // 将变量加载到寄存器。
            ret += generate_load(reg_x, reg_y, argument);
    }

    // 将序号大于 8 的参数存入栈中。
    for (uint32_t i = 8; i < call_inst.args.len; i++)
    {
        auto argument =
            reinterpret_cast<koopa_raw_value_t>(call_inst.args.buffer[i]);

        std::string reg_x = rm.reg_x; // 保存值的寄存器。
        std::string reg_y = rm.reg_y; // 保存偏移量的寄存器。
        // 将参数写入寄存器。
        if (argument->kind.tag == KOOPA_RVT_INTEGER)
        {
            // 将立即数写入寄存器。
            ret += fmt::format("    li {}, {}\n", reg_x,
                               argument->kind.data.integer.value);
        }
        else // 将变量加载到寄存器。
            ret += generate_load(reg_x, reg_y, argument);

        // 将寄存器中的参数写入栈。
        ret += generate_store(reg_x, reg_y, sfm.offset_lower() + (i - 8) * 4);
    }

    // 生成 call 指令。
    ret += fmt::format("    call {}\n", call_inst.callee->name + 1);

    // 如果函数有返回值，将返回值保存。
    if (parent_value->ty->tag != KOOPA_RTT_UNIT)
        ret += generate_store(rm.reg_ret, rm.reg_x, parent_value);

    return ret;
}
std::string visit(const koopa_raw_global_alloc_t& global_alloc_inst,
                  const koopa_raw_value_t& parent_value)
{
    std::string ret;

    gvm.alloc(parent_value, parent_value->name + 1);

    ret += "    .data\n";
    ret += fmt::format("    .globl {}\n", parent_value->name + 1);
    ret += fmt::format("{}:\n", parent_value->name + 1);

    ret += generate_global_init(global_alloc_inst.init);

    ret += "\n";

    return ret;
}
std::string visit(const koopa_raw_get_elem_ptr_t& get_elem_ptr_inst,
                  const koopa_raw_value_t& parent_value)
{
    const auto& source = get_elem_ptr_inst.src;
    const auto& index = get_elem_ptr_inst.index;
    return visit_array_or_pointer(source, index, parent_value);
}
std::string visit(const koopa_raw_get_ptr_t& get_ptr_inst,
                  const koopa_raw_value_t& parent_value)
{
    const auto& source = get_ptr_inst.src;
    const auto& index = get_ptr_inst.index;
    return visit_array_or_pointer(source, index, parent_value);
}
std::string visit_array_or_pointer(const koopa_raw_value_t& source,
                                   const koopa_raw_value_t& index,
                                   const koopa_raw_value_t& parent_value)
{
    std::string ret;

    std::string reg_x = rm.reg_x; // 存放数组的基地址。最终存放结果。
    std::string reg_y = rm.reg_y; // 最终存放偏移量。
    std::string reg_z = rm.reg_z; // 临时寄存器。

    // 加载数组基地址。
    {
        if (gvm.count(source))
            ret += fmt::format("    la {}, {}\n", reg_x, gvm.at(source));
        else
        {
            auto offset = sfm.offset(source);
            if (-2048 <= offset && offset < 2048)
                ret += fmt::format("    addi {}, sp, {}\n", reg_x, offset);
            else // 太大，使用 li 指令代替立即数。
            {
                ret += fmt::format("    li {}, {}\n", reg_z, offset);
                ret += fmt::format("    add {}, sp, {}\n", reg_x, reg_z);
            }
        }
    }

    // 计算偏移量。
    {
        // 将下标存入 reg_y。
        {
            // 如果是常量，将整数写入寄存器。
            if (index->kind.tag == KOOPA_RVT_INTEGER)
                ret += fmt::format("    li {}, {}\n", reg_y,
                                   index->kind.data.integer.value);
            else // 将变量加载到寄存器。
                ret += generate_load(reg_y, reg_z, index);
        }
        // 将数组大小存入 reg_z。
        {
            size_t size = get_size(source->ty);
            ret += fmt::format("    li {}, {}\n", reg_z, size);
        }
        // 计算偏移量，保存至 reg_y。
        ret += fmt::format("    mul {}, {}, {}\n", reg_y, reg_y, reg_z);
    }

    // 计算最终结果。
    ret += fmt::format("    add {}, {}, {}\n", reg_x, reg_x, reg_y);

    // 保存结果到栈帧。
    ret += generate_store(reg_x, reg_z, parent_value);

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
