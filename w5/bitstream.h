#pragma once

#include <cstddef>
#include <cstring>

class Bitstream
{
public:
    Bitstream(uint8_t* data) : ptr(data), offset(0) {}

    template<typename T>
    void write(const T& val)
    {
        memcpy(ptr + offset, reinterpret_cast<const uint8_t*>(&val), sizeof(T));
        offset += sizeof(T);
    }

    template<typename T>
    void read(T& val)
    {
        memcpy(reinterpret_cast<uint8_t*>(&val), ptr + offset, sizeof(T));
        offset += sizeof(T);
    }

private:
    uint8_t* ptr;
    uint32_t offset;
};
