// initial skeleton is a clone from https://github.com/jpcy/bgfx-minimal-example
//
#include <functional>
#include "raylib.h"
#include <enet/enet.h>
#include <math.h>

#include <vector>
#include "entity.h"
#include "protocol.h"
#include <cstdio>

#include "quantisation.h"
#include "bitstream.h"
#include <cassert>
#include <random>


static std::vector<Entity> entities;
static uint16_t my_entity = invalid_entity;

void on_new_entity_packet(ENetPacket *packet)
{
  Entity newEntity;
  deserialize_new_entity(packet, newEntity);
  // TODO: Direct adressing, of course!
  for (const Entity &e : entities)
    if (e.eid == newEntity.eid)
      return; // don't need to do anything, we already have entity
  entities.push_back(newEntity);
}

void on_set_controlled_entity(ENetPacket *packet)
{
  deserialize_set_controlled_entity(packet, my_entity);
}

void on_snapshot(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  Vector2 pos{}; float ori = 0.f;
  deserialize_snapshot(packet, eid, pos, ori);
  // TODO: Direct adressing, of course!
  for (Entity &e : entities)
    if (e.eid == eid)
    {
      e.pos = pos;
      e.ori = ori;
    }
}

bool quantized_equal(float x, float y, float lo, float hi, int num_bits)
{
  int range = (1 << num_bits) - 1;
  float step = (hi - lo) / range;
  return abs(x - y) < step;
}

void test_quantisation()
{
  // Float quantization
  // Unit
  float testFloat = 0.123f;
  PackedFloat<uint8_t, 4> floatPacked(testFloat, -1.f, 1.f);
  auto unpackedFloat = floatPacked.unpack(-1.f, 1.f);
  assert(quantized_equal(unpackedFloat, testFloat, -1.f, 1.f, 4));

  float2 testFloat2 = {4.f, -5.f};
  float2 interval = {-10.f, 10.f};
  PackedFloat2<uint16_t, 8, 8> packed_vec2(testFloat2, interval, interval);
  auto unpackedFloat2 = packed_vec2.unpack(interval, interval);
  assert(quantized_equal(unpackedFloat2.x, testFloat2.x, interval.x, interval.y, 8));
  assert(quantized_equal(unpackedFloat2.y, testFloat2.y, interval.x, interval.y, 8));

  interval = {-1.f, 1.f};
  float3 testFloat3 = {0.1f, -0.2f, 0.3f};
  PackedFloat3<uint32_t, 11, 10, 11> packed_vec3(testFloat3, interval, interval, interval);
  auto unpackedFloat3 = packed_vec3.unpack(interval, interval, interval);
  assert(quantized_equal(unpackedFloat3.x, testFloat3.x, interval.x, interval.y, 11));
  assert(quantized_equal(unpackedFloat3.y, testFloat3.y, interval.x, interval.y, 10));
  assert(quantized_equal(unpackedFloat3.z, testFloat3.z, interval.x, interval.y, 11));

  // Uint packing
  // Unit

  uint8_t* mem = (uint8_t *)malloc(sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t));
  Bitstream bsw{mem};
  bsw.writePackedUint(42u);
  bsw.writePackedUint(1337u);
  bsw.writePackedUint(420691337u);

  Bitstream bsr{mem};
  uint32_t val = 0;
  bsr.readPackedUint(val);
  assert(val == 42);
  bsr.readPackedUint(val);
  assert(val == 1337);
  bsr.readPackedUint(val);
  assert(val == 420691337);
}

int main(int argc, const char **argv)
{
  test_quantisation();

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
    float dt = GetFrameTime();
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
      for (Entity &e : entities)
        if (e.eid == my_entity)
        {
          // Update
          float thr = (up ? 1.f : 0.f) + (down ? -1.f : 0.f);
          float steer = (left ? -1.f : 0.f) + (right ? 1.f : 0.f);

          // Send
          send_entity_input(serverPeer, my_entity, thr, steer);
        }
    }

    BeginDrawing();
      ClearBackground(GRAY);
      BeginMode2D(camera);
        DrawRectangleLines(-16, -8, 32, 16, GetColor(0xff00ffff));
        for (const Entity &e : entities)
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
