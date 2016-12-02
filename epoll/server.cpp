#include <iostream>
//./server 127.0.0.1 12345 5


#include <sys/epoll.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/time.h>
#include <sys/select.h>
#include <poll.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>

//using namespace std;

#define BUFFER_SIZE 1024
#define HEAD_LEN 4
#define MAX_FD 1023
#define EPOOL_FD 1024
#define BUF_LEN 1000



int setnonblocking( int fd )
{
    int old_option = fcntl( fd, F_GETFL );
    int new_option = old_option | O_NONBLOCK;
    fcntl( fd, F_SETFL, new_option );
    return old_option;
}

void addfd( int epollfd, int fd )
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN;
    epoll_ctl( epollfd, EPOLL_CTL_ADD, fd, &event );
    setnonblocking( fd );
}


void tsocket(int argc, const char * argv[]);

int main(int argc, const char * argv[]) {
    tsocket(argc,argv);
    return 0;
}
void tsocket(int argc, const char * argv[]){
    if(argc < 3){
        exit(-1);
    }
    
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    int backlog = atoi(argv[3]);
    
    std::cout << "ip=" << ip << " port="<<port << " backlog=" << backlog  << std::endl;
    
    //声明信息
    char sockBuf[BUFFER_SIZE];

    //接受数据
    size_t ret;

    int fd;
    int check_ret;
    fd = socket(PF_INET,SOCK_STREAM , 0);
    assert(fd >= 0);

    //端口重用
    int   opt   =   1;   
    int   optLen   =   sizeof(opt);  
    setsockopt(fd, SOL_SOCKET,   SO_REUSEADDR,   &opt,   optLen);

    struct sockaddr_in address;
    bzero(&address,sizeof(address));
    //转换成网络地址
    address.sin_port = htons(port);
    address.sin_family = AF_INET;
    //地址转换
    inet_pton(AF_INET, ip, &address.sin_addr);
    //绑定ip和端口
    check_ret = bind(fd,(struct sockaddr*)&address,sizeof(address));
    assert(check_ret >= 0);

    //非阻塞的socket
    int cflags = fcntl(fd,F_GETFL,0);
    fcntl(fd,F_SETFL, cflags|O_NONBLOCK);

    //创建监听队列，用来存放待处理的客户连接
    check_ret = listen(fd, backlog);
    assert(check_ret >= 0);
    struct sockaddr_in addressClient;
    socklen_t clientLen = sizeof(addressClient);

    //使用epoll
    epoll_event epollArr[MAX_FD];

    int epollfd = epoll_create(EPOOL_FD);
    assert(epollfd != -1);
    addfd(epollfd,fd);
    setnonblocking(fd);

    int len;
    int socketfd;
    while(1){
        int number = epoll_wait(epollfd,epollArr, MAX_FD, -1);
        if( number < 0){
            printf("epoll error\n");
            break;
        }
        for(int i=0;i<number;++i){
            socketfd = epollArr[i].data.fd;
            //如果是监听的端口放入监听里面
            if(socketfd == fd){
                int connfd = accept(fd,NULL,NULL);
                addfd( epollfd, connfd );
            }else if(epollArr[i].events & EPOLLIN){
                int sockfd = epollArr[i].data.fd;
                while(1){
                    bzero(sockBuf,'\0');
                    len = read(socketfd,sockBuf,BUF_LEN);
                    if(len < 0){
                        close(socketfd);
                        break;
                    }else if(len == 0){
                        close(socketfd);
                    }else{
                        printf("receve:%s\n", sockBuf);
                        write(socketfd,sockBuf,len);
                    }
                }
            }else{
                printf("other happen\n");
            }
        }
        
    }
    close(fd);
}
