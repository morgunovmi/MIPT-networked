#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <cstring>
#include <cstdio>
#include <iostream>


#include "socket_tools.h"
#include "protocol.h"

int main(int argc, const char **argv)
{
  std::string ident{};
  if (argc > 2)
  {
    printf("Too many arguments");
    return 1;
  } else if (argc == 2)
  {
    ident = {argv[1]};
  } else
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

  while (true)
  {
    std::string input;
    printf(">");
    std::getline(std::cin, input);
    if (send_message(sfd, MT_DATA, input, resAddrInfo) != 0)
      return 1;
  }
  return 0;
}
