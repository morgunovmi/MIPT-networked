#include <enet/enet.h>
#include <iostream>
#include "entity.h"
#include "protocol.h"
#include "mathUtils.h"
#include <stdlib.h>
#include <vector>
#include <map>
#include <unordered_map>
#include <random>
#include "time.h"

static std::unordered_map<uint16_t, Entity> entities;
static std::map<uint16_t, ENetPeer*> controlledMap;

std::random_device rd{};
std::default_random_engine gen{rd()};
std::uniform_real_distribution<float> posDistr{-20.f, 20.f};
std::uniform_int_distribution<uint8_t> colorDistr{0, 255};
std::uniform_real_distribution<float> angleDistr{0, PI};

void on_join(ENetPeer *peer, ENetHost *host, uint32_t timestamp)
{
  // send all entities
  for (const auto &[eid, ent] : entities)
    send_new_entity(peer, ent);

  // find max eid
  uint16_t maxEid = entities.empty() ? invalid_entity : entities[0].eid;
  for (const auto &[eid, entity] : entities)
    maxEid = std::max(maxEid, eid);
  uint16_t newEid = maxEid + 1;
  Color color = {
    colorDistr(gen),
    colorDistr(gen),
    colorDistr(gen),
    255
  };
  Vector2 pos = {
    .x = posDistr(gen),
    .y = posDistr(gen)
  };

  Entity ent = {color, pos, 0.f, angleDistr(gen), 0.f, 0.f, newEid, timestamp};
  entities[newEid] = ent;

  controlledMap[newEid] = peer;


  // send info about new entity to everyone
  for (size_t i = 0; i < host->peerCount; ++i)
    send_new_entity(&host->peers[i], ent);
  // send info about controlled entity
  send_set_controlled_entity(peer, newEid);
}

void on_input(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  float thr = 0.f; float steer = 0.f;
  deserialize_entity_input(packet, eid, thr, steer);
  for (auto &[entity_id, entity] : entities)
    if (entity_id == eid)
    {
      entity.thr = thr;
      entity.steer = steer;
    }
}

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = 10131;

  ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

  if (!server)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  uint32_t lastTime = enet_time_get();
  while (true)
  {
    uint32_t curTime = enet_time_get();
    float dt = (curTime - lastTime) * 0.001f;
    lastTime = curTime;
    ENetEvent event;
    while (enet_host_service(server, &event, 0) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        switch (get_packet_type(event.packet))
        {
          case E_CLIENT_TO_SERVER_JOIN:
            on_join(event.peer, server, time_to_tick(curTime));
            break;
          case E_CLIENT_TO_SERVER_INPUT:
            on_input(event.packet);
            break;
        };
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }

    std::vector<EntityState> snapshot;
    for (auto &[eid, e] : entities)
    {
      // simulate
      for (; e.tick < time_to_tick(curTime); ++e.tick)
      {
        simulate_entity(e, dt);
      }

      snapshot.push_back({e.eid, e.pos, e.ori});
    }

    // send
    for (size_t i = 0; i < server->peerCount; ++i)
    {
      ENetPeer *peer = &server->peers[i];
      send_snapshot(peer, time_to_tick(curTime), snapshot);
    }
    usleep(static_cast<useconds_t>(1.f / TICKRATE * 1000000.f));
  }

  enet_host_destroy(server);

  atexit(enet_deinitialize);
  return 0;
}


