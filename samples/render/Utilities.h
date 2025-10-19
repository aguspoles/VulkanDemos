#pragma once

namespace prm
{
    template <class T>
    uint32_t to_u32(T value)
    {
        static_assert(std::is_arithmetic<T>::value, "T must be numeric");

        if (static_cast<uintmax_t>(value) > static_cast<uintmax_t>(std::numeric_limits<uint32_t>::max()))
        {
            throw std::runtime_error("to_u32() failed, value is too big to be converted to uint32_t");
        }

        return static_cast<uint32_t>(value);
    }

    // from: https://stackoverflow.com/a/57595105
    template <typename T, typename... Rest>
    void hashCombine(std::size_t& seed, const T& v, const Rest&... rest) {
        seed ^= std::hash<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
        (hashCombine(seed, rest), ...);
    };
}


