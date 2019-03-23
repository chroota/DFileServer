#pragma once
#include "tcp_server.hpp"

class Http:TcpServer
{
private:

public:
    Http();
    ~Http();

    bool listen(int port);
    void handle(char buf[]);
};
