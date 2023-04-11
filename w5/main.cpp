// initial skeleton is a clone from https://github.com/jpcy/bgfx-minimal-example
//
#include <functional>
#include "raylib.h"
#include <enet/enet.h>
#include <math.h>

#include <cstdio>
#include <vector>
#include <unordered_map>
#include <deque>
#include "entity.h"
#include "protocol.h"
#include "time.h"

static std::unordered_map<uint16_t, Entity> entities;
static std::unordered_map<uint16_t, std::deque<EntitySnapshot>> entitiesSnapshots;
static uint16_t my_entity = invalid_entity;

void on_new_entity_packet(ENetPacket *packet)
{
  Entity newEntity;
  deserialize_new_entity(packet, newEntity);
  if (!entities.contains(newEntity.eid))
  {
    entities[newEntity.eid] = std::move(newEntity);
    entitiesSnapshots[newEntity.eid].emplace_back(newEntity.tick, newEntity.pos, newEntity.ori);
  }
}

void on_set_controlled_entity(ENetPacket *packet)
{
  deserialize_set_controlled_entity(packet, my_entity);
}

void on_snapshot(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  EntitySnapshot snapshot{};
  deserialize_snapshot(packet, eid, snapshot);
  snapshot.tick += time_to_tick(OFFSET_MS);
  entitiesSnapshots[eid].push_back(snapshot);

  // printf("Received snapshot : %d : %f : %f : %f\n", eid, entity.pos.x, entity.pos.y, entity.ori);
}

void interpolate()
{
  uint32_t curTime = enet_time_get();
  for (auto &[eid, e] : entities)
  {
    auto &snapshots = entitiesSnapshots[eid];
    auto &snapshot_a = snapshots.front();
    if (time_to_tick(curTime) < snapshot_a.tick)
    {
      e.pos = snapshot_a.pos;
      e.ori = snapshot_a.ori;
      continue;
    }
    auto &snapshot_b = snapshots[1];
    if (time_to_tick(curTime) >= snapshot_b.tick)
    {
      snapshots.pop_front();
    }
    auto &first = snapshots.front();
    auto &second = snapshots[1];
    const auto t = static_cast<float>(time_to_tick(curTime) - first.tick) / (second.tick - first.tick);
    e.pos.x = std::lerp(first.pos.x, second.pos.x, t);
    e.pos.y = std::lerp(first.pos.y, second.pos.y, t);
    e.ori = std::lerp(first.ori, second.ori, t);
  }
}

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }

  ENetHost *client = enet_host_create(nullptr, 1, 2, 0, 0);
  if (!client)
  {
    printf("Cannot create ENet client\n");
    return 1;
  }

  ENetAddress address;
  enet_address_set_host(&address, "localhost");
  address.port = 10131;

  ENetPeer *serverPeer = enet_host_connect(client, &address, 2, 0);
  if (!serverPeer)
  {
    printf("Cannot connect to server");
    return 1;
  }

  int width = 600;
  int height = 600;

  InitWindow(width, height, "w5 networked MIPT");

  const int scrWidth = GetMonitorWidth(0);
  const int scrHeight = GetMonitorHeight(0);
  if (scrWidth < width || scrHeight < height)
  {
    width = std::min(scrWidth, width);
    height = std::min(scrHeight - 150, height);
    SetWindowSize(width, height);
  }

  Camera2D camera = { {0, 0}, {0, 0}, 0.f, 1.f };
  camera.target = Vector2{ 0.f, 0.f };
  camera.offset = Vector2{ width * 0.5f, height * 0.5f };
  camera.rotation = 0.f;
  camera.zoom = 10.f;


  SetTargetFPS(60);               // Set our game to run at 60 frames-per-second

  bool connected = false;
  while (!WindowShouldClose())
  {
    ENetEvent event;
    while (enet_host_service(client, &event, 0) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        send_join(serverPeer);
        connected = true;
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        switch (get_packet_type(event.packet))
        {
        case E_SERVER_TO_CLIENT_NEW_ENTITY:
          on_new_entity_packet(event.packet);
          break;
        case E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY:
          on_set_controlled_entity(event.packet);
          break;
        case E_SERVER_TO_CLIENT_SNAPSHOT:
          on_snapshot(event.packet);
          break;
        };
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }
    if (my_entity != invalid_entity)
    {
      bool left = IsKeyDown(KEY_LEFT);
      bool right = IsKeyDown(KEY_RIGHT);
      bool up = IsKeyDown(KEY_UP);
      bool down = IsKeyDown(KEY_DOWN);
      // TODO: Direct adressing, of course!
      auto &this_entity = entities[my_entity];
      float thr = (up ? 1.f : 0.f) + (down ? -1.f : 0.f);
      float steer = (left ? -1.f : 0.f) + (right ? 1.f : 0.f);

      // Send
      send_entity_input(serverPeer, my_entity, {this_entity.tick, thr, steer});
    }
    interpolate();

    BeginDrawing();
      ClearBackground(GRAY);
      BeginMode2D(camera);
        for (const auto &[eid, e] : entities)
        {
          const Rectangle rect = {e.pos.x, e.pos.y, 3.f, 1.f};
          DrawRectanglePro(rect, {0.f, 0.5f}, e.ori * 180.f / PI, e.color);
        }

      EndMode2D();
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
