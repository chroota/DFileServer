// #include <thread>
// #include <sys/epoll.h>
// #include <unistd.h>
// #include <string.h>
// #include <stdio.h>
// #include <fcntl.h>
// #include <sys/types.h>
// #include <sys/stat.h>

// using namespace std;
// #define MAXBUFSIZE 100

// // int fd = 7;
// int pfd[2];

// void writeFn(int pfd[2]){
//     int i=0;
//     char wbuf[20] = {0};
//     while(1){
//         sprintf(wbuf, "hello:%d", i++);
//         close(pfd[0]);
//         write(pfd[1], wbuf, strlen(wbuf));
//         sleep(1);
//     }
    

//     close(pfd[0]);
// }

// void readFn(int pfd[2]){
//     char rbuf[20] = {0};
//     while(1){
//         close(pfd[1]);
//         read(pfd[0], rbuf, 20);
//         puts(rbuf);
//         sleep(1);
//     }
// }

// int main(){
//     // char readbuf[MAXBUFSIZE] = {0};
//     // char writeubuf[MAXBUFSIZE] = "test write buf\n";
//     // write(fd, writeubuf, strlen(writeubuf));
//     // read(fd, readbuf, MAXBUFSIZE);
//     // printf("%s\n", readbuf);
//     // // puts(readbuf);

//     if(pipe(pfd) < 0){
//         puts("create pipe fail");
//         return -1;
//     }

//     thread thw(writeFn, pfd);
//     thread thr(readFn, pfd);

//     thw.join();
//     thr.join();
//     // printf("read:%d write:%d\n", pfd[0], pfd[1]);
// }

#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>
#include<stdlib.h>
#include<sys/mman.h>

int main()
{
    // int fd[2]; // 用来保存文件描述符
    // pipe(fd);

    // pid_t pid = fork();// 创建进程
    // if(pid > 0)
    // {
    //     // 父进程写管道，需要关闭读端
    //     close(fd[0]);
    //     int i = 0;
    //     for(i=10; i<200; i++)
    //     {
    //         write(fd[1], &i, sizeof(int));
    //         sleep(1);
    //     }

    //     close(fd[1]);// 关闭写端
    //     exit(0);
    // }

    // // 子进程读管道
    // close(fd[1]); // 先关闭写端 
    // int x;
    // int i = 0;
    // for(; i<100; i++)
    // {
    //     read(fd[0], &x, sizeof(int));
    //     printf("%d ", x);
    //     setbuf(stdout, NULL);
    // }
    // close(fd[0]);
    // printf("\n");

    int fd = open("./test_dir/t1.txt", );

    return 0;
}