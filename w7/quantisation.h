#pragma once
#include "mathUtils.h"
#include <limits>
#include <iostream>
#include <bit>

struct float2
{
  float x;
  float y;
};

struct float3
{
  float x;
  float y;
  float z;
};

template <typename T>
T pack_float(float v, float lo, float hi, int num_bits)
{
  T range = (1 << num_bits) - 1; // std::numeric_limits<T>::max();
  return range * ((clamp(v, lo, hi) - lo) / (hi - lo));
}

template <typename T>
float unpack_float(T c, float lo, float hi, int num_bits)
{
  T range = (1 << num_bits) - 1; // std::numeric_limits<T>::max();
  return float(c) / range * (hi - lo) + lo;
}

template <typename T, int num_bits>
  requires std::unsigned_integral<T>
struct PackedFloat
{
  static_assert(num_bits <= std::numeric_limits<T>::digits);
  T packedVal;

  PackedFloat(float v, float lo, float hi) { pack(v, lo, hi); }
  PackedFloat(T compressed_val) : packedVal(compressed_val) {}

  void pack(float v, float lo, float hi) { packedVal = pack_float<T>(v, lo, hi, num_bits); }
  float unpack(float lo, float hi) { return unpack_float<T>(packedVal, lo, hi, num_bits); }
};

template <typename T, int num_bits_x, int num_bits_y>
  requires std::unsigned_integral<T>
struct PackedFloat2
{
  static_assert(num_bits_x + num_bits_y <= std::numeric_limits<T>::digits);
  T packedVal;

  PackedFloat2(float x, float y, float lo_x, float hi_x, float lo_y, float hi_y)
  {
    pack(x, y, lo_x, hi_x, lo_y, hi_y);
  }
  PackedFloat2(float2 v, float2 bounds_x, float2 bounds_y)
  {
    pack(v.x, v.y, bounds_x.x, bounds_x.y, bounds_y.x, bounds_y.y);
  }
  PackedFloat2(T compressed_val) : packedVal(compressed_val) {}

  void pack(float x, float y, float lo_x, float hi_x, float lo_y, float hi_y)
  {
    const auto packedX = pack_float<T>(x, lo_x, hi_x, num_bits_x);
    const auto packedY = pack_float<T>(y, lo_y, hi_y, num_bits_y);

    packedVal = packedX << num_bits_y | packedY;
  }
  void pack(float2 v, float2 bounds_x, float2 bounds_y)
  {
    pack(v.x, v.y, bounds_x.x, bounds_x.y, bounds_y.x, bounds_y.y);
  }

  float2 unpack(float lo_x, float hi_x, float lo_y, float hi_y)
  {
    float2 ret{};
    ret.x = unpack_float<T>(packedVal >> num_bits_y, lo_x, hi_x, num_bits_x);
    ret.y = unpack_float<T>(packedVal & ((1 << num_bits_y) - 1), lo_y, hi_y, num_bits_y);
    return ret;
  }

  float2 unpack(float2 bounds_x, float2 bounds_y)
  {
    return unpack(bounds_x.x, bounds_x.y, bounds_y.x, bounds_y.y);
  }
};

template <typename T, int num_bits_x, int num_bits_y, int num_bits_z>
  requires std::unsigned_integral<T>
struct PackedFloat3
{
  static_assert(num_bits_x + num_bits_y + num_bits_z <= std::numeric_limits<T>::digits);
  T packedVal;

  PackedFloat3(float x, float y, float z,
               float lo_x, float hi_x,
               float lo_y, float hi_y,
               float lo_z, float hi_z)
  {
    pack(x, y, z, lo_x, hi_x, lo_y, hi_y, lo_z, hi_z);
  }
  PackedFloat3(float3 v, float2 bounds_x, float2 bounds_y, float2 bounds_z)
  {
    pack(v.x, v.y, v.z, bounds_x.x, bounds_x.y, bounds_y.x, bounds_y.y, bounds_z.x, bounds_z.y);
  }
  PackedFloat3(T compressed_val) : packedVal(compressed_val) {}

  void pack(float x, float y, float z,
            float lo_x, float hi_x,
            float lo_y, float hi_y,
            float lo_z, float hi_z)
  {
    const auto packedX = pack_float<T>(x, lo_x, hi_x, num_bits_x);
    const auto packedY = pack_float<T>(y, lo_y, hi_y, num_bits_y);
    const auto packedZ = pack_float<T>(z, lo_z, hi_z, num_bits_z);

    packedVal = (packedX << (num_bits_y + num_bits_z))
                | (packedY << num_bits_z)
                | packedZ;
  }
  void pack(float3 v, float2 bounds_x, float2 bounds_y, float2 bounds_z)
  {
    pack(v.x, v.y, v.z, bounds_x.x, bounds_x.y, bounds_y.x, bounds_y.y, bounds_z.x, bounds_z.y);
  }

  float3 unpack(float lo_x, float hi_x, float lo_y, float hi_y, float lo_z, float hi_z)
  {
    float3 ret{};
    ret.x = unpack_float<T>(packedVal >> (num_bits_y + num_bits_z), lo_x, hi_x, num_bits_x);
    ret.y = unpack_float<T>((packedVal >> num_bits_z) & ((1 << num_bits_y) - 1), lo_y, hi_y, num_bits_y);
    ret.z = unpack_float<T>(packedVal & ((1 << num_bits_z) - 1), lo_z, hi_z, num_bits_z);
    return ret;
  }

  float3 unpack(float2 bounds_x, float2 bounds_y, float2 bounds_z)
  {
    return unpack(bounds_x.x, bounds_x.y, bounds_y.x, bounds_y.y, bounds_z.x, bounds_z.y);
  }
};

using float4bitsQuantized = PackedFloat<uint8_t, 4>;