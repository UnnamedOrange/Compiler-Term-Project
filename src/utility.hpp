/**
 * @file utility.hpp
 * @author UnnamedOrange
 * @brief Utility classes.
 *
 * @copyright Copyright (c) UnnamedOrange. Licensed under the MIT License.
 * See the LICENSE file in the repository root for full license text.
 */

#pragma once

#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <string>

#include <fmt/core.h>

/**
 * @brief RTTI C file wrapper.
 */
class c_file
{
private:
    FILE* file{};

public:
    c_file() noexcept = default;
    c_file& operator=(const c_file&) = delete;
    c_file(const c_file&) = delete;
    c_file& operator=(c_file&& other) noexcept
    {
        if (this != &other)
        {
            file = other.file;
            other.file = nullptr;
        }
        return *this;
    }
    c_file(c_file&& other) noexcept : file{other.file} { other.file = nullptr; }

public:
    ~c_file() noexcept
    {
        try
        {
            close();
        }
        catch (...)
        {
        }
    }

public:
    /**
     * @brief Open a file using C open function.
     * If failed, throw an std::runtime_error.
     */
    static c_file open(const std::filesystem::path& filename,
                       const std::string& mode)
    {
        c_file ret;
        ret.file = std::fopen(filename.c_str(), mode.c_str());
        if (!ret.file)
            throw std::runtime_error("Failed to open file.");
        return ret;
    }

public:
    /**
     * @brief Close the file. If no file has been opened, do nothing.
     * If failed, throw an std::runtime_error.
     */
    void close()
    {
        int result = 0;
        if (file)
        {
            result = fclose(file);
        }
        if (result)
            throw std::runtime_error("Failed to close file.");
    }
    /**
     * @brief Get C FILE*.
     */
    operator FILE*() const noexcept { return file; }
};
