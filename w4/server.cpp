#include <enet/enet.h>
#include <iostream>
#include "entity.h"
#include "protocol.h"
#include <stdlib.h>
#include <vector>
#include <map>
#include "raymath.h"
#include <random>

static std::vector<Entity> entities;
static std::map<uint16_t, ENetPeer*> controlledMap;
static std::map<uint16_t, Vector2> aiTargets;

std::random_device rd{};
std::default_random_engine gen{rd()};
std::uniform_real_distribution<float> posDistr{-600.f, 600.f};
std::uniform_real_distribution<float> playerPosDistr{-100.f, 100.f};
std::uniform_int_distribution<uint8_t> colorDistr{0, 255};
std::uniform_real_distribution<float> sizeDistr{20.f, 50.f};

const uint16_t TICKRATE = 60;

void on_join(ENetPacket *packet, ENetPeer *peer, ENetHost *host)
{
  // send all entities
  for (const Entity &ent : entities)
    send_new_entity(peer, ent);

  // find max eid
  uint16_t maxEid = entities.empty() ? invalid_entity : entities[0].eid;
  for (const Entity &e : entities)
    maxEid = std::max(maxEid, e.eid);
  uint16_t newEid = maxEid + 1;
  Color color = {colorDistr(gen),
                 colorDistr(gen),
                 colorDistr(gen),
                 255};
  Vector2 pos
  {
    .x = playerPosDistr(gen),
    .y = playerPosDistr(gen)
  };
  float size = sizeDistr(gen);
  Entity ent = {color, pos, size, newEid};
  entities.push_back(ent);

  controlledMap[newEid] = peer;

  // send info about new entity to everyone
  for (size_t i = 0; i < host->peerCount; ++i)
    send_new_entity(&host->peers[i], ent);
  // send info about controlled entity
  send_set_controlled_entity(peer, newEid);
}

void on_state(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  Vector2 pos;
  deserialize_entity_state(packet, eid, pos);
  for (Entity &e : entities)
    if (e.eid == eid)
    {
      e.pos = pos;
    }
}

void generate_ai_entities()
{
  for (uint16_t i = 0; i < 10; ++i)
  {
    Color color = {colorDistr(gen),
                  colorDistr(gen),
                  colorDistr(gen),
                  255};
    Vector2 pos
    {
      .x = posDistr(gen),
      .y = posDistr(gen)
    };
    float size = sizeDistr(gen);
    
    Entity ent = {color, pos, size, i};
    entities.push_back(ent);

    Vector2 target {
      .x = posDistr(gen),
      .y = posDistr(gen)
    };
    aiTargets[i] = target;
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

  generate_ai_entities();

  while (true)
  {
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
            on_join(event.packet, event.peer, server);
            break;
          case E_CLIENT_TO_SERVER_STATE:
            on_state(event.packet);
            break;
        };
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }
    static int t = 0;
    for (Entity &e : entities)
    {
      for (Entity &e_two : entities)
      {
        if (e_two.eid != e.eid)
        {
          if (Vector2Distance(e.pos, e_two.pos) < e.size + e_two.size)
          {
            if (e.size > e_two.size)
            {
              e.size = std::min(e.size + e_two.size / 2.f, 300.f);
              e_two.size = std::max(e_two.size / 2.f, 20.f);
              e_two.pos =
              {
                .x = posDistr(gen),
                .y = posDistr(gen)
              };

              if (controlledMap.contains(e_two.eid))
              {
                send_snapshot(controlledMap[e_two.eid], e_two.eid, e_two.pos, e_two.size);
              }
              if (controlledMap.contains(e.eid))
              {
                send_snapshot(controlledMap[e.eid], e.eid, e.pos, e.size);
              }
            }
          }
        }
      }

      if (aiTargets.contains(e.eid))
      {
        if (Vector2Distance(aiTargets[e.eid], e.pos) < 1.f)
        {
          Vector2 target {
            .x = posDistr(gen),
            .y = posDistr(gen)
          };
          aiTargets[e.eid] = target;
        }
        Vector2 dir = Vector2Normalize(Vector2Subtract(aiTargets[e.eid], e.pos));
        e.pos.x += dir.x * 1 / TICKRATE * 100.f;
        e.pos.y += dir.y * 1 / TICKRATE * 100.f;
      }

      for (size_t i = 0; i < server->peerCount; ++i)
      {
        ENetPeer *peer = &server->peers[i];
        if (!controlledMap.contains(e.eid) || controlledMap[e.eid] != peer)
          send_snapshot(peer, e.eid, e.pos, e.size);
      }
    }
    usleep(static_cast<useconds_t>(1.f / TICKRATE * 1000000.f));
  }

  enet_host_destroy(server);

  atexit(enet_deinitialize);
  return 0;
}


