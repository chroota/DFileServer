#include "tcp_server.hpp"
int TcpServer::create_sock(int port)
{
    int listenfd;
    struct sockaddr_in local_addr;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        printf("Socket created failed\n");
        return -1;
    }

    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(port);
    local_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(listenfd, (struct sockaddr *) &local_addr, sizeof(local_addr)) <0)
    {
        printf("bind failed.\n");
        return -1;
    }

    if (!set_nonblocking(listenfd)) 
    {
        puts("fail to set nonblock");
        return -1;
    }

    if(listen(listenfd, LISTENQ) == -1)
    {
        puts("fail to listen");
        return -1;
    }

    return listenfd;
}

bool TcpServer::set_nonblocking(int fd)
{
    int sock_flags = fcntl(fd, F_GETFL, 0);
    sock_flags |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, sock_flags) != -1;
};
 
bool TcpServer::start(int port=DEFAULT_PORT)
{
    struct epoll_event event, events[max_events];
    int sockfd = create_sock(port);
    if (sockfd == -1) return false;
    
    int epfd= epoll_create(max_events);
    if(epfd == -1)
    {
        puts("fail to create epoll");
        return false;
    }

    event.data.fd = sockfd;
    event.events = EPOLLIN | EPOLLET;
    bool ret = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event);

    if (ret == -1) 
    {
        puts("fail to add into epoll");
        return false;
    }

    std::cout<<"waiting connect"<< std::endl;

    int connfd;
    char buf[MAXBUFSIZE];
    sockaddr_in client_addr;
    socklen_t client_addr_len;
    while(true)
    {
        int n = epoll_wait(epfd, events, max_events,-1);
        std::cout<<"connect come"<<std::endl;
        for(int i = 0; i < n; i++)
        {
            if(sockfd == events[i].data.fd)  //accept
            {
                connfd = accept(sockfd, (sockaddr *) &client_addr, &client_addr_len);
                if(connfd < 0 )
                {
                    puts("accept error");
                    continue;
                }

                char *addr_str = inet_ntoa(client_addr.sin_addr);
                printf("accept a connection from %s\n", addr_str);
                event.data.fd = connfd;
                event.events = EPOLLIN|EPOLLET;
                epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &event);

                time_t ticks;
                ticks = time(NULL);

                sprintf(buf, "fuck: %.24s \r\n", ctime(&ticks));
                write(connfd, buf, strlen(buf));
            }
            else if(events[i].events & EPOLLIN)   //handle evet
            {
                char readbuf[MAXBUFSIZE];
                printf("data comming\n");
                //handle
                int client_sockfd = events[i].data.fd;
                if (client_sockfd < 0) 
                {
                    continue;
                }
                int n;
                if ((n=read(client_sockfd, readbuf, MAXBUFSIZE)) <0 ) 
                {
                    //read error
                    printf("fuck error\n");
                }
                readbuf[n] = '\0';
                handle(readbuf);
                //printf("%s\n", buf);
            }else
            {
                puts("error in epoll event");
                close(events[i].data.fd);
                return false;
            }
        }   
    }
    return true;
}