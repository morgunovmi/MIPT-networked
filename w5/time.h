#pragma once

#include <cstdint>

constexpr uint16_t TICKRATE = 64;
constexpr float FIXED_DT_MS = 1.0f / TICKRATE * 1000.0f;
const int OFFSET_MS = 100 + FIXED_DT_MS;

int time_to_tick(float time)
{
  return static_cast<int>(time / FIXED_DT_MS);
}