#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <cstring>
#include <cstdio>
#include <vector>

#include "socket_tools.h"
#include "protocol.h"

int main(int argc, const char **argv)
{
  const char *port = "2022";

  int sfd = create_dgram_socket(nullptr, port, nullptr);
  if (sfd == -1)
    return 1;
  printf("listening!\n");

  const char *client_port = "2050";
  addrinfo clientAddrInfo{};
  int cl_sfd = 0;

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
        char *data = buffer + sizeof(message_type);
        switch (type)
        {
          case MT_INIT:
          {
            printf("New user joined - Ident: %s\n", data);
            cl_sfd = create_dgram_socket("localhost", client_port, &clientAddrInfo);
            if (cl_sfd == -1)
              return 1;
          }
          break;
          case MT_DATA:
            printf("Data message: %s\n", data);
            if (send_message(cl_sfd, MT_DATA, data, clientAddrInfo) != 0)
              return 1;
            break;
          case MT_KEEP_ALIVE:
            break;
          default:
            printf("Unsupported message type\n");
        }
      }
    }
  }
  return 0;
}
