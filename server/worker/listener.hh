

#pragma once
#include <arpa/inet.h>
#include <glog/logging.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <iostream>
#include <string>

class Listener
{
  public:
    Listener();
    Listener(std::string IP, std::string port);
    ~Listener();
    int Accept();
    std::string IP_;
    std::string port_;
    bool OK = false;
    int socketfd() const
    {
        return socket_fd;
    }

  private:
    int socket_fd;
    struct sockaddr_in serverAddress_;
};
