#pragma once

#include <cstdio>
#include <netdb.h>
#include <sys/types.h>
#include <string>
#include <iostream>
#include <cstring>

enum message_type
{
  MT_INIT,
  MT_DATA,
  MT_KEEP_ALIVE
};

int send_message(int fd, message_type type, std::string data, addrinfo addrInfo)
{
  char* message = (char *)malloc(data.length() + sizeof(message_type));
  message[0] = (char)type;
  memcpy(message + sizeof(message_type), data.c_str(), data.length());                            
  ssize_t res = sendto(fd, message, data.size() + sizeof(message_type), 0, addrInfo.ai_addr, addrInfo.ai_addrlen);
  if (res == -1)
  {
    std::cout << strerror(errno) << std::endl;
    return 1;
  }
  return 0;
}