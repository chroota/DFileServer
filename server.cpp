#include "server.hpp"

Server::Server(){

}

Server::~Server(){

}

int Server::create_sock(int port){
    int sock_fd;
    if (sock_fd = socket(AF_INET, SOCK_STREAM, 0) == -1) {
        perror("fail create socket");
        exit(1);
    }

    long flag;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(flag));

    struct sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(port);
    local_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(local_addr.sin_zero), 8);

    if (bind(sock_fd, (struct sockaddr *) &local_addr, sizeof(struct sockaddr)) == -1) {
        perror("fail to bind socket");
        exit(1);
    }

    if (!set_nonblocking(sock_fd)) {
        perror("fail to set nonblock");
    }

    if(listen(sock_fd, max_connection) == -1){
        perror("fail to listen");
        exit(1);
    }
    return sock_fd;
}

bool Server::set_nonblocking(int fd)
{
    int sock_flags = fcntl(fd, F_GETFL, 0);
    sock_flags |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, sock_flags) != -1;
};
 
bool Server::start(int port){
    struct epoll_event events[max_events];
    server_sock_fd = create_sock(port);
    if((epoll_fd = epoll_create1(0)) == -1){
        perror("fail to create epoll");
        exit(1);
    }

    event.data.fd = server_sock_fd;
    event.events = EPOLLIN | EPOLLET;
    bool ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_sock_fd, &event);

    if (!ret) {
        perror("fail to add into epoll");
        exit(1);
    }

    std::cout<<"waiting connect"<< std::endl;

    while(true){
        int n = epoll_wait(epoll_fd, events, max_events,-1);
        std::cout<<"connect come"<<std::endl;
        for(int i = 0; i < n; i++)
        {
            if(events[i].events & EPOLLERR || 
                events[i].events & EPOLLHUP || 
                !(events[i].events &EPOLLIN))
            {
                perror("error in epoll event");
                close(events[i].data.fd);
            }
            else if(server_sock_fd == events[i].data.fd)  //accept
            {
            }
            else   //handle evet
            {
                
            }
        }
        
    }
    
    
}