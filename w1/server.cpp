#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <chrono>

#include "socket_tools.h"
#include "protocol.h"

int main(int argc, const char **argv)
{
  const char *port = "2022";

  addrinfo resAddrInfo;
  int sfd = create_dgram_socket(nullptr, port, &resAddrInfo);

  if (sfd == -1)
    return 1;
  printf("listening!\n");

  auto last_heartbeat = std::chrono::high_resolution_clock::now();
  while (true)
  {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(sfd, &readSet);

    timeval timeout = { 0, 100000 }; // 100 ms
    select(sfd + 1, &readSet, NULL, NULL, &timeout);

    if (FD_ISSET(sfd, &readSet))
    {
      constexpr size_t buf_size = 1000;
      static char buffer[buf_size];
      memset(buffer, 0, buf_size);

      ssize_t numBytes = recvfrom(sfd, buffer, buf_size - 1, 0, nullptr, nullptr);
      if (numBytes > 0)
      {
        const auto type = (message_type)*buffer;
        char* data = buffer + sizeof(message_type);
        switch (type)
        {
          case MT_INIT:
            printf("New user joined - Ident: %s\n", data);
            break;
          case MT_DATA:
            printf("Data message: %s\n", data);
            break;
          default:
            printf("Couldn't decode message_type\n");
        }
      }
    }

    if (std::chrono::duration<double, std::milli>{
            std::chrono::high_resolution_clock::now() - last_heartbeat}
            .count() > 10000)
    {
      if (send_message(sfd, MT_KEEP_ALIVE, {}, resAddrInfo) != 0)
        return 1;
      last_heartbeat = std::chrono::high_resolution_clock::now();
    }
  }
  return 0;
}
