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
static std::unordered_map<uint16_t, std::deque<EntitySnapshot>> entitySnapshots;
static std::vector<EntitySnapshot> localSnapshotHistory;
static std::vector<EntityInput> localInputHistory;
static uint16_t my_entity = invalid_entity;

void on_new_entity_packet(ENetPacket *packet)
{
  Entity newEntity;
  deserialize_new_entity(packet, newEntity);
  if (!entities.contains(newEntity.eid))
  {
    entities[newEntity.eid] = std::move(newEntity);
    entitySnapshots[newEntity.eid].emplace_back(newEntity.tick + time_to_tick(OFFSET_MS),
                                                newEntity.pos, newEntity.ori);
  }
}

void on_set_controlled_entity(ENetPacket *packet)
{
  deserialize_set_controlled_entity(packet, my_entity);
}

void clear_snapshot_history(uint32_t tick)
{
  std::erase_if(localSnapshotHistory, [tick](auto &snapshot){ return snapshot.tick < tick; });
  std::erase_if(localInputHistory, [tick](auto &input){ return input.tick < tick; });
}

void resimulate_me(EntitySnapshot snapshot)
{
  auto &this_entity = entities[my_entity];
  this_entity.pos = snapshot.pos;
  this_entity.ori = snapshot.ori;
  for (const auto &input : localInputHistory)
  {
    this_entity.thr = input.thr;
    this_entity.steer = input.steer;
    simulate_entity(this_entity, FIXED_DT_MS * 0.001f);
  }
}

void on_snapshot(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  EntitySnapshot snapshot{};
  deserialize_snapshot(packet, eid, snapshot);
  snapshot.tick += time_to_tick(OFFSET_MS);
  if (eid == my_entity)
  {
    clear_snapshot_history(snapshot.tick);
    auto local_snapshot = localSnapshotHistory.front();
    if (local_snapshot.ori != snapshot.ori
        || local_snapshot.pos.x != snapshot.pos.x
        || local_snapshot.pos.y != snapshot.pos.y)
    {
      resimulate_me(snapshot);
    }
  }
  else 
  {
    entitySnapshots[eid].push_back(snapshot);
  }
}

void interpolate()
{
  uint32_t curTime = enet_time_get();
  uint32_t curTick = time_to_tick(curTime);
  
  for (auto &[eid, e] : entities)
  {
    if (entitySnapshots[eid].size() < 2)
      continue;

    if (curTick > entitySnapshots[eid][1].tick)
      entitySnapshots[eid].pop_front();

    auto &snapshot_a = entitySnapshots[eid].front();
    auto &snapshot_b = entitySnapshots[eid][1];
    const auto t = static_cast<float>(time_to_tick(curTime) - snapshot_a.tick) 
                                      / (snapshot_b.tick - snapshot_a.tick);
    e.pos.x = std::lerp(snapshot_a.pos.x, snapshot_b.pos.x, t);
    e.pos.y = std::lerp(snapshot_a.pos.y, snapshot_b.pos.y, t);
    e.ori = std::lerp(snapshot_a.ori, snapshot_b.ori, t);
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
  uint32_t connectTime = enet_time_get();
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
      this_entity.thr = thr;
      this_entity.steer = steer;

      for (; this_entity.tick < time_to_tick(enet_time_get()); ++this_entity.tick)
      {
        simulate_entity(this_entity, FIXED_DT_MS * 0.001f);
        localInputHistory.emplace_back(this_entity.tick, this_entity.thr, this_entity.steer);
        localSnapshotHistory.emplace_back(this_entity.tick, this_entity.pos, this_entity.ori);
      }
      // Send
      send_entity_input(serverPeer, my_entity, {this_entity.tick, thr, steer});
    }
    if (enet_time_get() > connectTime + OFFSET_MS)
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
