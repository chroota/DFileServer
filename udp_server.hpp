#pragma once
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include "defines.hpp"

#define DEFAULT_PORT 8080

class UdpServer
{
private:
    int createSock(int port);
public:
    UdpServer();
    ~UdpServer();
    bool start(int port, bool isResponse = true);
    virtual bool handle(char recvbuf[], int recv_len, char sendbuf[], int &send_len)=0;
    virtual bool handle(char recvbuf[], int recv_len)=0;
};
