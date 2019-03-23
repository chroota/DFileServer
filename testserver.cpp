#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#define LISTENQ 1
#define MAXLINE 512

int main(){
    int listenfd, connfd;
    socklen_t len;
    struct sockaddr_in servaddr, cliaddr;
    char buf[MAXLINE];
    char readbuf[MAXLINE];
    time_t ticks;
    int n;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        printf("Socket created failed\n");
        return -1;
    }

    //sleep(1000);

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(6666);
    servaddr.sin_addr.s_addr = (INADDR_ANY);

    if(bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) <0){
        printf("bind failed.\n");
        return -1;
    }

    printf("listening...\n");
    listen(listenfd, LISTENQ);

    while(1){
        len = sizeof(cliaddr);
        connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &len);
        printf("connect from %s, port %d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
        ticks = time(NULL);

        sprintf(buf, "fuck: %.24s \r\n", ctime(&ticks));
        write(connfd, buf, strlen(buf));

        while((n=read(connfd, readbuf, MAXLINE)) > 0){
            readbuf[n] = 0;
            puts(readbuf);
        }

        close(connfd);
       // sleep(1000);
    }    
}