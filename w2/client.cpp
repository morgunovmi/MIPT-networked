#include <enet/enet.h>
#include <iostream>
#include <cstring>
#include <thread>
#include <sstream>

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }

  ENetHost *client = enet_host_create(nullptr, 2, 2, 0, 0);
  if (!client)
  {
    printf("Cannot create ENet client\n");
    return 1;
  }

  ENetAddress address;
  enet_address_set_host(&address, "localhost");
  address.port = 10887;

  ENetPeer *lobbyPeer = enet_host_connect(client, &address, 2, 0);
  if (!lobbyPeer)
  {
    printf("Cannot connect to lobby");
    return 1;
  }

  ENetPeer *gamePeer = nullptr;

  std::thread inputThread{[&gamePeer, &lobbyPeer](){
    while (!gamePeer) {
      std::string input;
      std::getline(std::cin, input);
      if (strcmp(input.data(), "start") == 0)
      {
        printf("Starting the game...\n");
        const char *msg = "start";
        ENetPacket *packet = enet_packet_create(msg, strlen(msg) + 1, ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(lobbyPeer, 0, packet);
        break;
      }
    }
  }};

  uint32_t timeStart = enet_time_get();
  uint32_t lastServerTimeSend = timeStart;
  while (true)
  {
    ENetEvent event;
    while (enet_host_service(client, &event, 10) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        printf("Packet received '%s'\n", event.packet->data);
        if (event.packet->data[0] == 'g')
        {
          uint16_t port = std::atoi((const char *)(event.packet->data + 2));

          ENetAddress address;
          enet_address_set_host(&address, "localhost");
          address.port = port;

          gamePeer = enet_host_connect(client, &address, 2, 0);
          if (!gamePeer)
          {
            printf("Cannot connect to game");
            return 1;
          }
          printf("Connecting to game server on port %u\n", port);
        }
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }
    if (gamePeer)
    {
      uint32_t curTime = enet_time_get();
      if (curTime - lastServerTimeSend > 3000)
      {
        std::stringstream time;
        time << "My time is : " << curTime << '\n';
        ENetPacket *packet = enet_packet_create(time.str().data(),
                                                time.str().length() + 1,
                                                ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(gamePeer, 0, packet);
        lastServerTimeSend = curTime;
      }
    }
  }
  enet_host_destroy(client);

  atexit(enet_deinitialize);
  inputThread.join();
  return 0;
}
