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
#include <type_traits>
#include <vector>

#include <fmt/core.h>

namespace compiler
{
    class type_array_t;
    class type_pointer_t;
    class type_function_t;

    class type_base_t : public std::enable_shared_from_this<type_base_t>
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

    public:
        std::shared_ptr<type_array_t> operator[](size_t array_size);
        std::shared_ptr<type_pointer_t> operator*();
        template <typename... type_t>
        std::shared_ptr<type_function_t> operator()(type_t&&... types);
    };

    class type_primary_t : public type_base_t
    {
    public:
        std::string type_name;

    public:
        type_primary_t() = default;
        type_primary_t(const std::string& type_name) : type_name(type_name) {}

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

    class type_function_t : public type_base_t
    {
    public:
        std::shared_ptr<type_base_t> return_type;
        std::vector<std::shared_ptr<type_base_t>> param_types;

    public:
        std::string to_koopa() const override
        {
            std::string param_string;
            for (size_t i = 0; i < param_types.size(); i++)
            {
                const auto& param = param_types[i];
                if (!param_string.empty())
                    param_string += ", ";
                param_string += fmt::format("{}", param->to_koopa());
            }

            std::string return_string;
            if (auto t = return_type->to_koopa(); !t.empty())
                return_string += fmt::format(": {}", t);

            return fmt::format("({}){}", param_string, return_string);
        }
        size_t size() const override { return 4; }
        std::shared_ptr<type_base_t> get_base_type() const override
        {
            return return_type;
        }
    };

    inline std::shared_ptr<type_array_t> type_base_t::operator[](
        size_t array_size)
    {
        auto ret = std::make_shared<type_array_t>();
        ret->base_type = shared_from_this();
        ret->array_size = array_size;
        return ret;
    }
    inline std::shared_ptr<type_pointer_t> type_base_t::operator*()
    {
        auto ret = std::make_shared<type_pointer_t>();
        ret->base_type = shared_from_this();
        return ret;
    }
    template <typename... type_t>
    inline std::shared_ptr<type_function_t> type_base_t::operator()(
        type_t&&... types)
    {
        auto ret = std::make_shared<type_function_t>();
        ret->return_type = shared_from_this();
        auto push_back = [&](auto& type) {
            using T = std::decay_t<decltype(type)>;
            if constexpr (std::is_base_of_v<type_base_t, T>)
                ret->param_types.push_back(type.shared_from_this());
            else
                ret->param_types.push_back(type);
        };
        (push_back(types), ...);
        return ret;
    }

    inline auto _shared_int_type = std::make_shared<type_primary_t>("int");
    inline auto _shared_void_type = std::make_shared<type_primary_t>("void");
    inline auto& int_type = *_shared_int_type;
    inline auto& void_type = *_shared_void_type;
} // namespace compiler
