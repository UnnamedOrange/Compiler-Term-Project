/**
 * @file main.cpp
 * @author UnnamedOrange
 * @brief Entry of the compiler.
 *
 * @copyright Copyright (c) UnnamedOrange. Licensed under the MIT License.
 * See the LICENSE file in the repository root for full license text.
 */

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#include <argparse/argparse.hpp>
#include <fmt/core.h>

#include <backend/koopa_to_riscv.h>
#include <frontend/sysy_to_koopa.h>
#include <global_variables.hpp>

#pragma region "Define default values for DEBUG."
/**
 * @brief Compiler mode.
 */
enum class compiler_mode_t
{
    unknown,
    /**
     * @brief Compile SysY to Koopa IR.
     */
    koopa,
    /**
     * @brief Compile SysY to RISC-V.
     */
    riscv,
    /**
     * @brief Compile SysY to RISC-V, with performance optimization.
     */
    perf,
};
/**
 * @brief Default compiler mode for debug.
 */
constexpr compiler_mode_t DEBUG_USE_COMPILER_MODE = compiler_mode_t::unknown;
/**
 * @brief Default input file path for debug.
 */
constexpr std::string_view DEBUG_USE_INPUT_FILE_PATH = "";
#pragma endregion

/**
 * @brief Entry of the compiler.
 *
 * @todo Modify perf mode.
 */
int main(int argn, char** argv)
{
    using namespace compiler;

    argparse::ArgumentParser program("compiler");
    compiler_mode_t mode = DEBUG_USE_COMPILER_MODE;

    // Define arguments.
    {
        program.add_argument("-koopa")
            .default_value(false)
            .implicit_value(true)
            .help("Run in koopa mode.");
        program.add_argument("-riscv")
            .default_value(false)
            .implicit_value(true)
            .help("Run in RISC-V mode.");
        program.add_argument("-perf")
            .default_value(false)
            .implicit_value(true)
            .help("Run in performance test mode.");

        program.add_argument("input")
            .required()
            .default_value(std::string(DEBUG_USE_INPUT_FILE_PATH))
            .metavar("INPUT_FILE")
            .help("Specify the input file.");

        program.add_argument("-o")
            .default_value(std::string("a.out"))
            .metavar("OUTPUT_FILE")
            .help("Specify the output file name.");
    }

    // Parse the arguments.
    {
        try
        {
            program.parse_args(argn, argv);
        }
        catch (const std::runtime_error& err)
        {
            std::cerr << err.what() << std::endl;
            std::cerr << program;
            std::exit(1);
        }
    }

    // Get mode from the arguments.
    {
        int mode_count = 0;
        mode_count += program.get<bool>("-koopa");
        mode_count += program.get<bool>("-riscv");
        mode_count += program.get<bool>("-perf");
        if ((DEBUG_USE_COMPILER_MODE != compiler_mode_t::unknown &&
             mode_count > 1) ||
            (DEBUG_USE_COMPILER_MODE == compiler_mode_t::unknown &&
             mode_count != 1))
        {
            std::cerr << "Please specify exactly one mode." << std::endl;
            std::cerr << program;
            std::exit(1);
        }
        if (program.get<bool>("-koopa"))
            mode = compiler_mode_t::koopa;
        else if (program.get<bool>("-riscv"))
            mode = compiler_mode_t::riscv;
        else if (program.get<bool>("-perf"))
            mode = compiler_mode_t::perf;
    }

    // Get file paths from the arguments.
    {
        global::input_file_path = program.get<std::string>("input");
        global::output_file_path = program.get<std::string>("-o");
    }

    // Compile.
    {
        sysy_to_koopa compiler_koopa;
        std::ofstream ofs(global::output_file_path);

        switch (mode)
        {
        case compiler_mode_t::koopa:
        {
            std::cout << fmt::format("[Main] Runs in Koopa mode.") << std::endl;
            auto koopa_ir_str = compiler_koopa.compile(global::input_file_path);
            ofs << koopa_ir_str << std::endl;
            break;
        }
        case compiler_mode_t::riscv:
        {
            std::cout << fmt::format("[Main] Runs in RISC-V mode.")
                      << std::endl;
            auto koopa_ir_str = compiler_koopa.compile(global::input_file_path);
            koopa_to_riscv compiler_riscv;
            auto riscv_str = compiler_riscv.compile(koopa_ir_str);
            ofs << riscv_str << std::endl;
            break;
        }
        case compiler_mode_t::perf:
        {
            std::cout << fmt::format("[Main] Runs in perf mode.") << std::endl;
            // TODO: Modify perf mode.
            auto koopa_ir_str = compiler_koopa.compile(global::input_file_path);
            koopa_to_riscv compiler_riscv;
            auto riscv_str = compiler_riscv.compile(koopa_ir_str);
            ofs << riscv_str << std::endl;
            break;
        }
        default:
            break;
        }
    }
}
