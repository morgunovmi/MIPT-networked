#include <enet/enet.h>
#include <iostream>
#include <cstring>
#include <unordered_set>

void sendServerMessage(const std::string& msg, const std::unordered_set<ENetPeer*> peers)
{
  for (const auto& peer: peers)
  {
    ENetPacket *packet = enet_packet_create(msg.data(),
                                            msg.length() + 1,
                                            ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 0, packet);
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
  address.port = 10887;

  ENetHost *gameLobby = enet_host_create(&address, 32, 2, 0, 0);

  std::unordered_set<ENetPeer*> connectedPlayers;
  constexpr uint16_t gameServerPort = 10900;
  std::string serverMsg{"g:10900"};
  bool isGameRunning = false;

  if (!gameLobby)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  while (true)
  {
    ENetEvent event;
    while (enet_host_service(gameLobby, &event, 10) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host,
               event.peer->address.port);
        if (isGameRunning)
        {
          ENetPacket *packet = enet_packet_create(serverMsg.data(),
                                                  serverMsg.length() + 1,
                                                  ENET_PACKET_FLAG_RELIABLE);
          enet_peer_send(event.peer, 0, packet);
        }
        connectedPlayers.insert(event.peer);
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        printf("Packet received '%s'\n", event.packet->data);
        if (strcmp((const char*)event.packet->data, "start") == 0)
        {
          isGameRunning = true;
          sendServerMessage(serverMsg, connectedPlayers);
        }
        enet_packet_destroy(event.packet);
        break;
      case ENET_EVENT_TYPE_DISCONNECT:
        printf("Connection with %x:%u ended\n", event.peer->address.host, event.peer->address.port);
        connectedPlayers.erase(event.peer);
        break;
      default:
        break;
      };
    }
  }

  enet_host_destroy(gameLobby);

  atexit(enet_deinitialize);
  return 0;
}

