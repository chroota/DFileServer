#pragma once
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

#define DEFAULT_PORT 8080
#define MAXBUFSIZE  512
#define LISTENQ 5

class TcpServer
{
private:
    const int max_connection = 10;
    const int max_events = 32;
    int create_sock(int port);
    bool set_nonblocking(int fd);
public:
    TcpServer(){};
    ~TcpServer(){};
    bool start(int port);
    virtual void handle(char buf[])=0;
};