/**
 * @file type_system.h
 * @author UnnamedOrange
 * @brief Type system.
 *
 * @copyright Copyright (c) UnnamedOrange. Licensed under the MIT License.
 * See the LICENSE file in the repository root for full license text.
 */

#pragma once

#include <memory>
#include <stdexcept>
#include <string>

#include <fmt/core.h>

namespace compiler
{
    class type_base_t
    {
    public:
        virtual ~type_base_t() = default;

    public:
        virtual std::string to_koopa() const = 0;
        virtual size_t size() const = 0;
        virtual std::shared_ptr<type_base_t> get_base_type() const
        {
            return std::shared_ptr<type_base_t>();
        }
    };

    class type_primary_t : public type_base_t
    {
    public:
        std::string type_name;

    public:
        std::string to_koopa() const override
        {
            if (false)
                ;
            else if (type_name == "int")
                return "i32";
            else if (type_name == "void")
                return "";
            throw std::domain_error("type_name is out of domain.");
        }
        size_t size() const override
        {
            if (false)
                ;
            else if (type_name == "int")
                return 4;
            else if (type_name == "void")
                return 0;
            throw std::domain_error("type_name is out of domain.");
        }
    };

    class type_array_t : public type_base_t
    {
    public:
        std::shared_ptr<type_base_t> base_type;
        size_t array_size;

    public:
        std::string to_koopa() const override
        {
            return fmt::format("[{}, {}]", base_type->to_koopa(), array_size);
        }
        size_t size() const override { return base_type->size() * array_size; }
        std::shared_ptr<type_base_t> get_base_type() const override
        {
            return base_type;
        }
    };

    class type_pointer_t : public type_base_t
    {
    public:
        std::shared_ptr<type_base_t> base_type;

    public:
        std::string to_koopa() const override
        {
            return fmt::format("*{}", base_type->to_koopa());
        }
        size_t size() const override { return 4; }
        std::shared_ptr<type_base_t> get_base_type() const override
        {
            return base_type;
        }
    };
} // namespace compiler
