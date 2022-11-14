/**
 * @file ast.h
 * @author UnnamedOrange
 * @brief Define AST types.
 *
 * @copyright Copyright (c) UnnamedOrange. Licensed under the MIT License.
 * See the LICENSE file in the repository root for full license text.
 */

#pragma once

#include <memory>

namespace compiler::ast
{
    class ast_base_t;
    /**
     * @brief General AST type using smart pointer.
     */
    using ast_t = std::shared_ptr<ast_base_t>;

    /**
     * @brief AST base class.
     *
     * @todo Design member functions.
     */
    class ast_base_t
    {
    public:
        virtual ~ast_base_t() = default;
    };

    /**
     * @brief AST of a complete program.
     *
     * @todo Design member variables.
     */
    class ast_program_t : public ast_base_t
    {
    };
} // namespace compiler::ast
