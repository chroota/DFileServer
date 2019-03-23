// #include <sys/inotify.h>  
// #include <unistd.h>  
// #include <string.h>  
// #include <stdio.h>  
  
// /* 
// struct inotify_event { 
//    int      wd;       // Watch descriptor  
//    uint32_t mask;     // Mask of events  
//    uint32_t cookie;   // Unique cookie associating related  events (for rename(2)) 
//    uint32_t len;      // Size of name field  
//    char     name[];   // Optional null-terminated name  
// }; 
 
// */  
  
// int watch_inotify_events(int fd)  
// {  
//     char event_buf[512];  
//     int ret;  
//     int event_pos = 0;  
//     int event_size = 0;  
//     struct inotify_event *event;  
      
//     /*读事件是否发生，没有发生就会阻塞*/  
//     ret = read(fd, event_buf, sizeof(event_buf));  
      
//     /*如果read的返回值，小于inotify_event大小出现错误*/  
//     if(ret < (int)sizeof(struct inotify_event))  
//     {  
//         printf("counld not get event!\n");  
//         return -1;  
//     }  
      
//     /*因为read的返回值存在一个或者多个inotify_event对象，需要一个一个取出来处理*/  
//     while( ret >= (int)sizeof(struct inotify_event) )  
//     {  
//         event = (struct inotify_event*)(event_buf + event_pos);  
//         if(event->len)  
//         {  
//             if(event->mask & IN_CREATE)  
//             {  
//                 printf("create file: %s\n",event->name);  
//             }  
//             else  
//             {  
//                 printf("delete file: %s\n",event->name);  
//             }  
//         }  
          
//         /*event_size就是一个事件的真正大小*/  
//         event_size = sizeof(struct inotify_event) + event->len;  
//         ret -= event_size;  
//         event_pos += event_size;  
//     }  
      
//     return 0;  
// }  
  
// int main(int argc, char** argv)  
// {  
//     int InotifyFd;  
//     int ret;  
      
//     if (argc != 2)  
//     {  
//         printf("Usage: %s <dir>\n", argv[0]);  
//         return -1;  
//     }  
      
//     /*inotify初始化*/  
//     InotifyFd = inotify_init();  
//     if( InotifyFd == -1)  
//     {  
//         printf("inotify_init error!\n");  
//         return -1;  
//     }  
      
//     /*添加watch对象*/  
//     ret = inotify_add_watch(InotifyFd, argv[1], IN_CREATE |  IN_DELETE | IN_MODIFY);  
 
//     /*处理事件*/  
//     watch_inotify_events(InotifyFd);  
  
//     /*删除inotify的watch对象*/  
//     if ( inotify_rm_watch(InotifyFd, ret) == -1)   
//     {  
//         printf("notify_rm_watch error!\n");  
//         return -1;  
//     }  
      
//     /*关闭inotify描述符*/  
//     close(InotifyFd);  
      
//     return 0;  
// }  


// #include <stdio.h>  
// #include <string.h>  
// #include <stdlib.h>  
// #include <sys/inotify.h>  
// #include <unistd.h>  
 
// #define EVENT_NUM 12  
 
// char *event_str[EVENT_NUM] =
// {
// 	"IN_ACCESS",
// 	"IN_MODIFY",
// 	"IN_ATTRIB",
// 	"IN_CLOSE_WRITE",
// 	"IN_CLOSE_NOWRITE",
// 	"IN_OPEN",
// 	"IN_MOVED_FROM",
// 	"IN_MOVED_TO",
// 	"IN_CREATE",
// 	"IN_DELETE",
// 	"IN_DELETE_SELF",
// 	"IN_MOVE_SELF"
// };
 
// int main(int argc, char *argv[])
// {
// 	int fd;
// 	int wd;
// 	int len;
// 	int nread;
// 	char buf[BUFSIZ];
// 	struct inotify_event *event;
// 	int i;
 
// 	if (argc < 2)
// 	{
// 		fprintf(stderr, "%s path\n", argv[0]);
// 		return -1;
// 	}
 
// 	fd = inotify_init();
// 	if (fd < 0)
// 	{
// 		fprintf(stderr, "inotify_init failed\n");
// 		return -1;
// 	}
 
// 	wd = inotify_add_watch(fd, argv[1], IN_CREATE | IN_DELETE | IN_MODIFY );
// 	if (wd < 0)
// 	{
// 		fprintf(stderr, "inotify_add_watch %s failed\n", argv[1]);
// 		return -1;
// 	}
 
// 	buf[sizeof(buf) - 1] = 0;
// 	while ((len = read(fd, buf, sizeof(buf) - 1)) > 0)
// 	{
// 		nread = 0;
// 		while (len > 0)
// 		{
// 			event = (struct inotify_event *)&buf[nread];
// 			for (i = 0; i<EVENT_NUM; i++)
// 			{
// 				if ((event->mask >> i) & 1)
// 				{
// 					if (event->len > 0)
// 						fprintf(stdout, "%s --- %s\n", event->name, event_str[i]);
// 					else
// 						fprintf(stdout, "%s --- %s\n", " ", event_str[i]);
// 				}
// 			}
// 			nread = nread + sizeof(struct inotify_event) + event->len;
// 			len = len - sizeof(struct inotify_event) - event->len;
// 		}

//         puts("test");
// 	}
 
// 	return 0;
// }


#include <stdio.h>
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <sys/types.h>  
#include <sys/stat.h>  
#include <fcntl.h>  
#include <unistd.h>  
#include <string.h>  
#include <stdlib.h>
 
/* 定义epoll最大监听的文件数量 */  
#define EPOLL_MAX_EVENTS    32
 
#define BUFFER_SIZE     1024
#define ARRAY_LENGTH    128     // 定义数组的长度  
#define NAME_LENGTH     128     // 定义文件名长度的限制 
 
struct file_name_fd_desc {  
    int fd;                         // 文件的描述符  
    char name[32];                  // 文件名  
    char base_name[NAME_LENGTH];    // 带绝对路径的文件名  
}; 
 
 
static struct epoll_event gEpollEventArray[EPOLL_MAX_EVENTS];
 
/* 定义一个数组用来存放对应文件的文件描述符和文件名 */  
static struct file_name_fd_desc gFileFdArray[ARRAY_LENGTH];
 
static int array_index = 0;
 
static char *base_dir;   
  
 
/*添加文件描述符到Epoll*/
static int add_to_epoll(int epoll_fd, int file_fd)
{
    int result;
    struct epoll_event eventItem;
    memset(&eventItem,0,sizeof(eventItem));
    eventItem.events=EPOLLIN;
    eventItem.data.fd = file_fd;
    result = epoll_ctl(epoll_fd,EPOLL_CTL_ADD,file_fd,&eventItem);   
    return result;
}
 
/*从Epoll删除文件描述符*/
static  void remove_epoll(int epoll_fd,int file_fd)
{
   epoll_ctl(epoll_fd,EPOLL_CTL_DEL,file_fd,NULL);
}
 
/*inotify监听到事件的处理逻辑，将新创建的文件添加到epoll,删除的文件从epoll删除*/
static int inotify_event_handler(int epoll_fd,int notify_fd)
{
    char InfoBuf[BUFFER_SIZE];
    struct inotify_event *event;
    char* p;
    int tmp_fd;
    int i;
  
    memset(InfoBuf,0,BUFFER_SIZE); 
    
    int result = read(notify_fd,InfoBuf,BUFFER_SIZE);
    for(p = InfoBuf ; p < InfoBuf + result;)
    {
       event = (struct inotify_event *)(p);
       if(event->mask & IN_CREATE)
       {
          sprintf(gFileFdArray[array_index].name,"%s",event->name);
          sprintf(gFileFdArray[array_index].base_name,"%s%s",base_dir,event->name);
          tmp_fd = open(gFileFdArray[array_index].base_name, O_RDWR);
          if(tmp_fd == -1)
          {
            printf("open file failure : %s\n",gFileFdArray[array_index].base_name);
            return -1;
          }
          gFileFdArray[array_index].fd = tmp_fd;
          add_to_epoll(epoll_fd,tmp_fd);
          array_index += 1;
          printf("add file to epoll %s\n",event->name);
       }else  //delete file
       {
          for(i = 0 ; i < ARRAY_LENGTH ; i++)
          { 
            if(!strcmp(gFileFdArray[i].name,event->name))
            {
               remove_epoll(epoll_fd,gFileFdArray[i].fd);
               gFileFdArray[i].fd = 0;
               memset(gFileFdArray[i].name, 0, sizeof(gFileFdArray[i].name));  
               memset(gFileFdArray[i].base_name, 0, sizeof(gFileFdArray[i].base_name));
               printf("delete file to epoll %s\n",event->name);
               break;  
            }
          }
       }
 
       p += sizeof(struct inotify_event) + event->len;    
    }
    
}
 
 
 
int main(int argc,char** argv)
{
   int mInotifyId;
   int mEpollId;
 
   char readbuf[1024];  
   int readlen;
   
   if(argc != 2)
   {
     printf("Paramter Error\n");
   }
   
   base_dir = argv[1];
 
   //epoll创建
   mEpollId = epoll_create(1);
   if(mEpollId == -1)
   {
      printf("Epoll Create Error\n");
      return -1;
   }    
   
   mInotifyId = inotify_init();
 
   //Observe Directory FILE_CREATE & FILE_DELETE
   //inotify添加对文件的监听
   int result = inotify_add_watch(mInotifyId,argv[1],IN_DELETE | IN_CREATE);
   if(result == -1)
   {
     printf("File Add Watch Failure\n");
     return -1;
   }
   
   add_to_epoll(mEpollId,mInotifyId);
   
   while(1)
   {
      result = epoll_wait(mEpollId,gEpollEventArray,EPOLL_MAX_EVENTS,-1);
      if(result == -1)
      {
         printf("epoll wait error\n");
         return -1;
      }
      else
      { 
         printf("file event happen\n");
         int i = 0; 
         for(i = 0; i < result; i++)
         {
            if(gEpollEventArray[i].data.fd == mInotifyId)
            {
                //inotify event handler
                if(-1 == inotify_event_handler(mEpollId, mInotifyId))  
                {  
                    printf("inotify handler error!\n");  
                    return -1;  
                }  
            
            }else
            {
                printf("read data.....\n");  
                //read content of file
                readlen = read(gEpollEventArray[i].data.fd, readbuf, 1024);  
                readbuf[readlen] = '\0';  
                printf("read data %s\n",readbuf);    
            }
         }
      }
   }
}


//完整demo代码
// #include <sys/inotify.h>
// #include <sys/epoll.h>
// #include <sys/ioctl.h>
// #include <sys/utsname.h>
// #include <unistd.h>
// #include <string.h>
// #include <stdio.h>
 
// int watch_inotify_events(int fd) {
//     char event_buf[512];
//     int ret;
//     int event_pos = 0;
//     int event_size = 0;
//     struct inotify_event *event;
//     /*读事件是否发生,没有发生则会阻塞*/
//     ret = read(fd, event_buf, sizeof(event_buf));
//     /*若read的返回值,小于inotify_event大小,说明我们连最基本的一个event都读取出来,是错误的结果*/
//     if(ret < (int)sizeof(struct inotify_event)) {
//         printf("read error,could get event");
//         return -1;
//     }
//     /*一次读取可能会去读取多个事件,需要一个循环全部读取出来*/
//     while(ret > (int)sizeof(struct inotify_event)) {
//         event = (struct inotify_event*)(event_buf + event_pos);
//         if(event->len) {
//             if(event->mask & IN_CREATE) {
//                 printf("create file:%s successfully \n", event->name);
//             } else {
//                 printf("delete file:%s successfully \n", event->name);
//             }
//         }
//         //event 的真实大小,name是可变的,所以要加上event->len
//         event_size = sizeof(struct inotify_event) + event->len;
//         ret -= event_size;
//         event_pos += event_size;
//     }
//     return 0;
// }
 
 
// int main(int argc, char** argv) {
//     int epollFd;
//     int inotifyFd;
//     int pendingEventCount;
//     int pendingEventIndex;
//     epollFd = epoll_create(8);
//     inotifyFd = inotify_init();
//     int result_notify = inotify_add_watch(inotifyFd, argv[1], IN_CREATE | IN_DELETE);
//     if(result_notify < 0) {
//         printf("Could not register INotify. \n");
//         return -1;
//     }
//     if(epollFd < 0) {
//         printf("Could not create epoll instance. \n");
//         return -1;
//     }
//     struct epoll_event eventItem;
//     eventItem.events = EPOLLIN;
//     int result_epoll = epoll_ctl(epollFd, EPOLL_CTL_ADD, inotifyFd, &eventItem);
//     if(result_epoll != 0) {
//         printf("Could not add INotify to epoll instance.  \n");
//     }
//     struct epoll_event pendingEventItems[16];
//     int pollResult = epoll_wait(epollFd, pendingEventItems, 16, 30*1000ll);
//     if(pollResult > 0) {
//         pendingEventCount = size_t(pollResult);
//     } else {
//         pendingEventCount = 0;
//     }
//     while(pendingEventIndex < pendingEventCount) {
//         const struct epoll_event& eventItem = pendingEventItems[pendingEventIndex++];
//         if (eventItem.events & EPOLLIN) {
//             watch_inotify_events(inotifyFd);
//         }
//     }
//     return 0;
// }
