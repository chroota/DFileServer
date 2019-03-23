#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>

#define MAXBUFFSIZE 256
#define PORT 6666
#define HOST_ADDR "127.0.0.1"

int main(){
    int sockfd, n;
    char recvbuff[MAXBUFFSIZE];
    char sendbuf[MAXBUFFSIZE];
    struct sockaddr_in servaddr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        printf("socket created failed\n");
    }
    
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(6666);
    printf("connecting...\n");

    if(connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
        printf("Connecting server failed.\n");
        return -1;
    }
    n=read(sockfd, recvbuff, MAXBUFFSIZE);
    recvbuff[n] = 0;
    puts(recvbuff);

    memset(sendbuf, 0, sizeof(sendbuf));

    //



    do
    {
        //fputs(recvbuff, stdout);
        scanf("%s", sendbuf);
        write(sockfd, sendbuf, strlen(sendbuf));
    } while (sendbuf[0] != 'x');

    close(sockfd);
    

    // while((n=read(sockfd, recvbuff, MAXBUFFSIZE))>0){
    //     recvbuff[n] = 0;
    //     fputs(recvbuff, stdout);
    // }

    // if(n<0){
    //     printf("Read failed!\n");
    //     return -2;
    // }

    return 0;
}