#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <thread>
#include <unistd.h>

#include "socket_tools.h"
#include "protocol.h"

int main(int argc, const char **argv)
{
  std::string ident{};
  if (argc > 2)
  {
    printf("Too many arguments");
    return 1;
  }
  else if (argc == 2)
  {
    ident = {argv[1]};
  }
  else
  {
    ident = {"anon"};
  }

  const char *port = "2022";

  addrinfo resAddrInfo;
  int sfd = create_dgram_socket("localhost", port, &resAddrInfo);
  if (sfd == -1)
  {
    printf("Cannot create a socket\n");
    return 1;
  }

  if (send_message(sfd, MT_INIT, ident, resAddrInfo) != 0)
    return 1;

  std::jthread heartbeat_thread{[](int sfd, addrinfo addrInfo)
                               {
                                 while (true)
                                 {
                                   sleep(10);
                                   if (send_message(sfd, MT_KEEP_ALIVE, {}, addrInfo) != 0)
                                     return;
                                 }
                               },
                               sfd, resAddrInfo};

  const char *port_rec = "2050";
  int sfd_rec = create_dgram_socket(nullptr, port_rec, nullptr);
  if (sfd_rec == -1)
  {
    printf("Couldn't create socket for listening\n");
    return 1;
  }

  std::jthread listen_thread{[](int sfd)
                            {
                              while (true)
                              {
                                fd_set readSet;
                                FD_ZERO(&readSet);
                                FD_SET(sfd, &readSet);

                                timeval timeout = {0, 100000}; // 100 ms
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
                                    case MT_DATA:
                                      printf("Server response: %s\n>", data);
                                      fflush(stdout);
                                      break;
                                    default:
                                      printf("Unsupported message type\n");
                                    }
                                  }
                                }
                              }
                            },
                            sfd_rec};

  printf(">");
  while (true)
  {
    std::string input;
    std::getline(std::cin, input);
    if (send_message(sfd, MT_DATA, input, resAddrInfo) != 0)
      return 1;
  }
  return 0;
}
