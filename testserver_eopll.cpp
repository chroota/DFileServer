#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>

#define LISTENQ 5
#define MAXLINE 512


bool set_nonblocking(int fd)
{
    int sock_flags = fcntl(fd, F_GETFL, 0);
    sock_flags |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, sock_flags) != -1;
};


int main(){
    int listenfd, connfd;
    socklen_t cli_len;
    sockaddr_in servaddr, cliaddr;
    char buf[MAXLINE];
    char readbuf[MAXLINE];
    time_t ticks;
    int n;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        printf("Socket created failed\n");
        return -1;
    }

    int epfd = epoll_create(256);
    epoll_event event, events[20];
    event.data.fd = listenfd;
    event.events = EPOLLIN|EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &event);

    //sleep(1000);

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(6666);
    servaddr.sin_addr.s_addr = (INADDR_ANY);

    if(bind(listenfd, (sockaddr *) &servaddr, sizeof(servaddr)) <0){
        printf("bind failed.\n");
        return -1;
    }

    printf("listening...\n");
    listen(listenfd, LISTENQ);

    int nfds;
    while(1){
        nfds = epoll_wait(epfd, events, 20, 500);
        for(int i = 0; i < nfds; i++)
        {
            if (events[i].data.fd == listenfd) {   //accept
                connfd = accept(listenfd, (sockaddr *)&cliaddr, &cli_len);
                if (connfd < 0) {
                    perror("connfd < 0");
                    exit(1);
                }

                char *str = inet_ntoa(cliaddr.sin_addr);
                printf("accept a connection from %s\n", str);
                event.data.fd = connfd;
                event.events = EPOLLIN|EPOLLET;

                epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &event);
                ticks = time(NULL);

                sprintf(buf, "fuck: %.24s \r\n", ctime(&ticks));
                write(connfd, buf, strlen(buf));
            }
            else if(events[i].events & EPOLLIN)
            {
                printf("data comming\n");
                //handle
                int client_sockfd = events[i].data.fd;
                if (client_sockfd < 0) {
                    continue;
                }
                int n;
                if ((n=read(client_sockfd, buf, MAXLINE)) <0 ) {
                    //read error
                    printf("fuck error\n");
                }

                buf[n] = '\0';
                printf("%s\n", buf);
            }
        }
    }    
}