#pragma once
#include <cstdint>
#include <vector>
#include "raylib.h"

constexpr uint16_t invalid_entity = -1;
struct EntityState
{
  uint16_t eid = invalid_entity;
  Vector2 pos;
  float ori;
};
using Snapshot = std::vector<EntityState>;

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

