#include <iostream>
#include <enet/enet.h>
#include <unordered_map>
#include <array>
#include <random>
#include <sstream>

constexpr std::array<const char *, 4> names = {"John", "Jane", "Robert", "Rebecca"};
constexpr std::array<const char *, 4> surnames = {"Smith", "Doe", "Johnson", "Williams"};

std::string generateRandomName() 
{ 
    std::random_device rd; 
    std::mt19937 generator{rd()}; 
    std::uniform_int_distribution<> name_dis(0, names.size() - 1); 
    std::uniform_int_distribution<> surname_dis(0, surnames.size() - 1); 
  
    std::stringstream ss;
    ss <<  names[name_dis(generator)] << " " << surnames[surname_dis(generator)];
  
    return ss.str(); 
} 

size_t generateId()
{
    std::random_device rd; 
    std::mt19937 generator{rd()}; 
    std::uniform_int_distribution<size_t> distr(0, std::numeric_limits<size_t>::max()); 
    return distr(generator);
}

struct PlayerInfo
{
    size_t id;
    std::string name;
};

using PlayerMap = std::unordered_map<ENetPeer*, PlayerInfo>;

void onNewPlayerJoined(ENetPeer* newPeer, PlayerMap& players)
{
    PlayerInfo newPlayer{generateId(), generateRandomName()};
    std::stringstream msg;
    msg << "New player joined:\nName: " << newPlayer.name << ", ID: " << newPlayer.id << '\n';

    std::stringstream playersTable;
    playersTable << "Current players:\n";
    for (const auto& [peer, info] : players)
    {
        playersTable << "Name: " << info.name << ", ID: " << info.id << '\n';

        ENetPacket *packet = enet_packet_create(msg.str().data(),
                                                msg.str().length() + 1,
                                                ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(peer, 0, packet);
    }

    ENetPacket *packet = nullptr;
    if (!players.empty())
    {
        packet = enet_packet_create(playersTable.str().data(),
                                    playersTable.str().length() + 1,
                                    ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(newPeer, 0, packet);
    }
    players[newPeer] = newPlayer;
    printf("Assigned name: %s and id: %zu\n", newPlayer.name.data(), newPlayer.id);
}

void sendPings(ENetPeer* peerToSend, const PlayerMap& players)
{
    for (const auto &[peer, info]: players)
    {
        if (peer != peerToSend)
        {
            std::stringstream msg;
            msg << info.name << "'s ping is : " << peer->roundTripTime << '\n';
            ENetPacket *packet = enet_packet_create(msg.str().data(),
                                                    msg.str().length() + 1,
                                                    ENET_PACKET_FLAG_UNSEQUENCED);
            enet_peer_send(peer, 1, packet);
        }
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
  address.port = 10900;

  ENetHost *gameServer = enet_host_create(&address, 42, 2, 0, 0);
  if (!gameServer)
  {
    printf("Cannot create ENet game server\n");
    return 1;
  }
  printf("Game server started\n");

  PlayerMap players;

  uint32_t timeStart = enet_time_get();
  uint32_t lastServerTimeSend = timeStart;
  while (true)
  {
    ENetEvent event;
    while (enet_host_service(gameServer, &event, 10) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        onNewPlayerJoined(event.peer, players);
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        printf("Packet received from id#%zu: '%s'\n", players[event.peer].id, event.packet->data);
        enet_packet_destroy(event.packet);
        break;
      case ENET_EVENT_TYPE_DISCONNECT:
        printf("Connection with %x:%u ended\n", event.peer->address.host, event.peer->address.port);
        players.erase(event.peer);
        break;
      default:
          break;
      };
    }
    if (!players.empty())
    {
      uint32_t curTime = enet_time_get();
      if (curTime - lastServerTimeSend > 3000)
      {
        for (const auto& [peer, info] : players)
        {
            std::stringstream time;
            time << "Server time is : " << curTime << '\n';
            ENetPacket *packet = enet_packet_create(time.str().data(),
                                                    time.str().length() + 1,
                                                    ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(peer, 0, packet);
            lastServerTimeSend = curTime;

            sendPings(peer, players);
        }
      }
    }
  }

  enet_host_destroy(gameServer);

  atexit(enet_deinitialize);
  return 0;
}
