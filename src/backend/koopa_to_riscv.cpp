/**
 * @file koopa_to_riscv.cpp
 * @author UnnamedOrange
 * @brief Compile Koopa IR to RISC-V.
 *
 * @copyright Copyright (c) UnnamedOrange. Licensed under the MIT License.
 * See the LICENSE file in the repository root for full license text.
 */

#include "koopa_to_riscv.h"

#include <cassert>
#include <iostream>

#include <fmt/core.h>

#if defined(COMPILER_LINK_KOOPA)
#include <koopa.h>
#endif

using namespace compiler;

#if defined(COMPILER_LINK_KOOPA)

std::string to_riscv(const std::string&);
std::string visit(const koopa_raw_program_t&);
std::string visit(const koopa_raw_slice_t&);
std::string visit(const koopa_raw_function_t&);
std::string visit(const koopa_raw_basic_block_t&);
std::string visit(const koopa_raw_value_t&);
std::string visit(const koopa_raw_return_t&);

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
    return fmt::format("    li a0, {}\n    ret\n",
                       return_inst.value->kind.data.integer.value);
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
