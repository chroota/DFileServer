#pragma once
#include "udp_server.hpp"
#include "msg.pb.h"

class NodeServer:UdpServer
{
private:

public:
    NodeServer(){};
    ~NodeServer(){};

    bool handle(char recvbuf[], int recv_len, char sendbuf[], int *send_len);

    bool listen(int port);
};
