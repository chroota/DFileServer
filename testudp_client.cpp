//socket udp 客户端
#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<string.h>
#include <stdio.h>
#define MAXBUFSIZE  512

int main()
{
    int sockfd=socket(AF_INET,SOCK_DGRAM,0);
    
    struct sockaddr_in addr;
    addr.sin_family =AF_INET;
    addr.sin_port =htons(8888);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    socklen_t len = sizeof(addr);

    char sendbuf[MAXBUFSIZE];
    char recvbuf[MAXBUFSIZE];
    sendbuf[0] = 0x01;
    sendto(sockfd, sendbuf, MAXBUFSIZE, 0, (struct sockaddr*)&addr,len);
    recvfrom(sockfd, recvbuf, MAXBUFSIZE,0,(struct sockaddr*)&addr,&len);
    puts(recvbuf);

    sendbuf[0] = 0x02;
    sendto(sockfd, sendbuf, MAXBUFSIZE, 0, (struct sockaddr*)&addr,len);
    recvfrom(sockfd, recvbuf, MAXBUFSIZE,0,(struct sockaddr*)&addr,&len);
    puts(recvbuf);

    sendbuf[0] = 0x03;
    sendto(sockfd, sendbuf, MAXBUFSIZE, 0, (struct sockaddr*)&addr,len);
    recvfrom(sockfd, recvbuf, MAXBUFSIZE,0,(struct sockaddr*)&addr,&len);
    puts(recvbuf);

    sendbuf[0] = 0x04;
    sendto(sockfd, sendbuf, MAXBUFSIZE, 0, (struct sockaddr*)&addr,len);
    recvfrom(sockfd, recvbuf, MAXBUFSIZE,0,(struct sockaddr*)&addr,&len);
    puts(recvbuf);



    // while(1)
    // {
    //     printf("what do you want to speak：");
    //     char buf[MAXBUFSIZE];
    //     scanf("%s", buf);
    //     sendto(sockfd, buf, MAXBUFSIZE, 0, (struct sockaddr*)&addr,len);

    //     recvfrom(sockfd, buf, MAXBUFSIZE,0,(struct sockaddr*)&addr,&len);
    //     puts(buf);
    // }
    close(sockfd);
}