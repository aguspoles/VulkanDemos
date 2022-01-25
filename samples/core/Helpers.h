#pragma once

namespace prm
{
    std::vector<char> read_shader_file(const std::string& filename);

    template <typename T>
    std::vector<uint8_t> to_bytes(const T& value)
    {
        return std::vector<uint8_t>{reinterpret_cast<const uint8_t*>(&value),
            reinterpret_cast<const uint8_t*>(&value) + sizeof(T)};
    }
}  
