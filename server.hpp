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

class Server
{
private:
    int server_sock_fd;
    int epoll_fd;
    struct epoll_event event;
    
    const int max_connection = 10;
    const int max_events = 32;


    int create_sock(int port);
    bool set_nonblocking(int fd);


public:
    Server();
    ~Server();
    bool start(int port);
};
