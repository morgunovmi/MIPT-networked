#include "protocol.h"
#include "bitstream.h"

void send_join(ENetPeer *peer)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t), ENET_PACKET_FLAG_RELIABLE);
  *packet->data = E_CLIENT_TO_SERVER_JOIN;

  enet_peer_send(peer, 0, packet);
}

void send_new_entity(ENetPeer *peer, const Entity &ent)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(Entity),
                                                   ENET_PACKET_FLAG_RELIABLE);
  Bitstream bs{packet->data};
  bs.write(E_SERVER_TO_CLIENT_NEW_ENTITY);
  bs.write(ent);

  enet_peer_send(peer, 0, packet);
}

void send_set_controlled_entity(ENetPeer *peer, uint16_t eid)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t),
                                                   ENET_PACKET_FLAG_RELIABLE);
  Bitstream bs{packet->data};
  bs.write(E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY);
  bs.write(eid);

  enet_peer_send(peer, 0, packet);
}

void send_entity_input(ENetPeer *peer, uint16_t eid, const EntityInput &input)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) +
                                                   sizeof(EntityInput),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  Bitstream bs{packet->data};
  bs.write(E_CLIENT_TO_SERVER_INPUT);
  bs.write(eid);
  bs.write(input);

  enet_peer_send(peer, 1, packet);
}

void send_snapshot(ENetPeer *peer, uint16_t eid, const EntitySnapshot &snapshot)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) +
                                                   sizeof(EntitySnapshot),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);

  Bitstream bs{packet->data};
  bs.write(E_SERVER_TO_CLIENT_SNAPSHOT);
  bs.write(eid);
  bs.write(snapshot);

  enet_peer_send(peer, 1, packet);
}

MessageType get_packet_type(ENetPacket *packet)
{
  return (MessageType)*packet->data;
}

void deserialize_new_entity(ENetPacket *packet, Entity &ent)
{
  MessageType type{};
  Bitstream bs{packet->data};
  bs.read(type);
  bs.read(ent);
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid)
{
  MessageType type{};
  Bitstream bs{packet->data};
  bs.read(type);
  bs.read(eid);
}

void deserialize_entity_input(ENetPacket *packet, uint16_t &eid, EntityInput &input)
{
  MessageType type{};
  Bitstream bs{packet->data};
  bs.read(type);
  bs.read(eid);
  bs.read(input);
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, EntitySnapshot &snapshot)
{
  MessageType type{};
  Bitstream bs{packet->data};
  bs.read(type);
  bs.read(eid);
  bs.read(snapshot);
}

