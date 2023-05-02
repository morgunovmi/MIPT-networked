#pragma once

#include <cstddef>
#include <cstring>
#include <vector>

inline bool is_little_endian() {
  return std::endian::native == std::endian::little;
}

#ifdef _MSC_VER

#include <stdlib.h>
#define bswap_16(x) _byteswap_ushort(x)
#define bswap_32(x) _byteswap_ulong(x)

#elif defined(__APPLE__)

// Mac OS X / Darwin features
#include <libkern/OSByteOrder.h>
#define bswap_16(x) OSSwapInt16(x)
#define bswap_32(x) OSSwapInt32(x)

#else

#include <byteswap.h>

#endif

static uint16_t bswap_if_little_endian(uint16_t v)
{
  return is_little_endian() ? bswap_16(v) : v;
}

static uint32_t bswap_if_little_endian(uint32_t v)
{
  return is_little_endian() ? bswap_32(v) : v;
}

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
    void write(const std::vector<T>& vec)
    {
        memcpy(ptr + offset, vec.data(), sizeof(T) * vec.size());
        offset += sizeof(T) * vec.size();
    }

    template<typename T>
    void read(T& val)
    {
        memcpy(reinterpret_cast<uint8_t*>(&val), ptr + offset, sizeof(T));
        offset += sizeof(T);
    }

    template<typename T>
    void read(std::vector<T>& vec, uint64_t num)
    {
        vec.resize(num);
        memcpy(vec.data(), ptr + offset, sizeof(T) * num);
        offset += sizeof(T) * num;
    }

    template <typename U>
        requires std::unsigned_integral<U>
    void writePackedUint(U v)
    {
        // First 2 bits encode type
        // 00 - u8, 01 - u16, 10 - u32
        if (v < (1 << 6)) // 64
        {
            *(ptr + offset) = uint8_t{v};
            offset += sizeof(uint8_t);
        }
        else if (v < (1 << 14)) // 16'384
        {
            const uint16_t res = (1 << 14) | v;
            *reinterpret_cast<uint16_t *>(ptr + offset) 
                = uint16_t{bswap_if_little_endian(res)};
            offset += sizeof(uint16_t);
        }
        else if (v < (1 << 30)) // 1'073'741'824
        {
            const uint32_t res = (1 << 31) | v;
            *reinterpret_cast<uint32_t *>(ptr + offset)
                 = uint32_t{bswap_if_little_endian(res)};
            offset += sizeof(uint32_t);
        }
        else
        {
            std::cerr << "Value to pack too big\n";
            *reinterpret_cast<uint32_t *>(ptr + offset)
                 = std::numeric_limits<U>::max();
            offset += sizeof(uint32_t);
        }
    }

    template <typename U>
        requires std::unsigned_integral<U>
    void readPackedUint(U &val)
    {
        switch (*(ptr + offset) >> 6)
        {
            case 0:
                val = (*(ptr + offset)) & 0x3F;
                offset += sizeof(uint8_t);
                break;
            case 1:
                val = bswap_if_little_endian((*(uint16_t *)(ptr + offset))) & 0x3FFF;
                offset += sizeof(uint16_t);
                break;
            case 2:
                val = bswap_if_little_endian((*(uint32_t *)(ptr + offset))) & 0x3FFFFFFF;
                offset += sizeof(uint32_t);
                break;
            default:
                std::cerr << "Unrecognised value\n";
                val = std::numeric_limits<U>::max();
                offset += sizeof(uint32_t);
        }
    }

    [[nodiscard]] uint32_t getOffset() { return offset; }

private:
    uint8_t* ptr;
    uint32_t offset;
};