#pragma once

#include <cstdint>

constexpr uint32_t TICKRATE = 64;
constexpr uint32_t FIXED_DT_MS = std::ceil(1.0f / TICKRATE * 1000.0f);
constexpr uint32_t OFFSET_MS = 100 + FIXED_DT_MS;

constexpr inline uint32_t time_to_tick(uint32_t time)
{
  return time / FIXED_DT_MS;
}