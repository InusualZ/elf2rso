#pragma once

#include <filesystem>
#include <fstream>
#include <string_view>

#include "swap.h"
#include "types.h"

class FileWriter
{
  private:
    std::ofstream filestream;

  public:
    FileWriter(std::filesystem::path filepath) : filestream(filepath, std::ios::binary) {}

    template <typename T>
    inline void writeBE(T data)
    {
        static_assert(std::is_arithmetic<T>::value,
                      "function only makes sense with arithmetic types");

        Common::swap<sizeof(data)>(reinterpret_cast<u8*>(&data));
        write(data);
    }

    template <typename T>
    inline void write(T data)
    {
        static_assert(std::is_arithmetic<T>::value,
                      "function only makes sense with arithmetic types");

        filestream.write(reinterpret_cast<const char*>(&data), sizeof(data));
    }

    inline void writeString(const std::string& data)
    {
        filestream.write(data.c_str(), data.size());
        filestream << '\0';  // Null terminated
    }

    inline void write(const char* data, size_t size) { filestream.write(data, size); }

    inline size_t position() { return static_cast<size_t>(filestream.tellp()); }

    inline void seek(size_t position) { filestream.seekp(static_cast<size_t>(position)); }

    inline void padToAlignment(size_t alignment)
    {
        if (alignment == 0)
        {
            return;
        }

        auto pos = position();
        auto count = (~(alignment - 1U) & (alignment + pos) - 1U) - pos;

        u8 b = 0;
        for (; count > 0; --count)
        {
            filestream.write(reinterpret_cast<const char*>(&b), sizeof(b));
        }
    }
};