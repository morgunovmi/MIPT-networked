#pragma once
#include <cstdint>
#include <vector>
#include "raylib.h"

constexpr uint16_t invalid_entity = -1;
struct EntitySnapshot
{
  uint32_t tick;
  Vector2 pos;
  float ori;
};
struct EntityInput
{
  uint32_t tick;
  float thr;
  float steer;
};

struct Entity
{
  Color color = {0, 255, 0, 255};
  Vector2 pos = {0.f, 0.f};
  float speed = 0.f;
  float ori = 0.f;

  float thr = 0.f;
  float steer = 0.f;

  uint16_t eid = invalid_entity;
  uint32_t tick = 0;
};

void simulate_entity(Entity &e, float dt);

