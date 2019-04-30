#include "udp_server.hpp"

int UdpServer::createSock(int port){
    int sockfd = socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd < 0) logger.fatal("error create socket");

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    int ret = bind(sockfd,(struct sockaddr*)&addr,sizeof(addr));
    if(ret < 0)
    {
        logger.log(L4, "error bind socket at port:%d", port);
        return -1;
    }
    return sockfd;
}

bool UdpServer::start(int port){
    int sockfd = createSock(port);
    sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);

    char recvbuf[MAXMSGSIZE];
    char sendbuf[MAXMSGSIZE];
    bool isResponse = true;
    int sentLen = 0;

    while(true)
    {
        memset(sendbuf, 0, MAXMSGSIZE);
        memset(recvbuf, 0, MAXMSGSIZE);
        int sendLen = 0;
        int recvLen = recvfrom(sockfd, recvbuf, sizeof(recvbuf), 0, (struct sockaddr*)&client_addr, &len);
        if(recvLen < 0)
        {
            logger.fatal("recv error");
            return false;
        }
        handle(recvbuf, strlen(recvbuf), sendbuf, sendLen, isResponse);
        if(isResponse)
        {
            sentLen = sendto(sockfd, sendbuf, sendLen, 0, (struct sockaddr*)&client_addr, len);
            if(sentLen < 0)
            {
                logger.fatal("send error");
                return false;
            }
        }
    }
    close(sockfd);
}