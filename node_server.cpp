#include "node_server.hpp"

bool NodeServer::handle(char recvbuf[], int recv_len, char sendbuf[], int *send_len)
{
    if (strlen(recvbuf) == 0 || recv_len <= 0) 
    {
        puts("error: len = 0");
        return false;
    }

    Msg::Message recvMsg;
    recvMsg.ParseFromArray(recvbuf, MAXBUFSIZE);

    if (recvMsg.type() == Msg::MasterNotifyCmd_Request) {
        
    }
    return true;
}

bool NodeServer::listen(int port){
    printf("listening at port: %d\n", port);
    if (!start(port)) {
        return false;
    }
    return true;
}