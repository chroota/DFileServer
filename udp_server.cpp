#include "udp_server.hpp"

UdpServer::UdpServer(){

}

UdpServer::~UdpServer(){

}

int UdpServer::createSock(int port){
    int sockfd=socket(AF_INET,SOCK_DGRAM,0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    int ret =bind(sockfd,(struct sockaddr*)&addr,sizeof(addr));
    if(ret < 0)
    {
        printf("bind error\n");
        return -1;
    }
    return sockfd;
}

bool UdpServer::start(int port, bool isResponse){
    int sockfd = createSock(port);
    sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);

    char recvbuf[MAXBUFSIZE];
    char sendbuf[MAXBUFSIZE];

    while(true){
        memset(sendbuf, 0, MAXBUFSIZE);
        memset(recvbuf, 0, MAXBUFSIZE);
        int send_len = 0;
        recvfrom(sockfd, recvbuf, sizeof(recvbuf), 0, (struct sockaddr*)&client_addr, &len);
        //printf("%d\n", strlen(recvbuf));
        handle(recvbuf, strlen(recvbuf), sendbuf, send_len);
        // sprintf(recvbuf, "recieve success");
        if (!isResponse) continue;
        sendto(sockfd, sendbuf, send_len, 0, (struct sockaddr*)&client_addr, len);
    }
}