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
     //   printf("bind error\n");
        logger.log(L4, "error bind socket at port:%d", port);
        return -1;
    }
    return sockfd;
}

bool UdpServer::start(int port){
    int sockfd = createSock(port);
    sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);

    char recvbuf[MAXBUFSIZE];
    char sendbuf[MAXBUFSIZE];
    bool isResponse = true;

    while(true)
    {
        memset(sendbuf, 0, MAXBUFSIZE);
        memset(recvbuf, 0, MAXBUFSIZE);
        int send_len = 0;
        recvfrom(sockfd, recvbuf, sizeof(recvbuf), 0, (struct sockaddr*)&client_addr, &len);
        handle(recvbuf, strlen(recvbuf), sendbuf, send_len, isResponse);
        if(isResponse)
        {
            sendto(sockfd, sendbuf, send_len, 0, (struct sockaddr*)&client_addr, len);
        }
    }

    close(sockfd);
}