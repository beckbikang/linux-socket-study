#include <iostream>
//./server 127.0.0.1 12345 5

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
#define BUF_LEN 1000


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
    int socketfd[MAX_FD];
    char data[BUFFER_SIZE+HEAD_LEN];

    //use pool
    int i,connfd,sockfd;
    int maxi;
    struct pollfd mypool[MAX_FD];

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

    //监听事件
    mypool[0].fd = fd;
    mypool[0].events = POLLIN ;

    for(i=1;i<MAX_FD;++i){
        mypool[i].fd = -1;
    }

    //运行监听
    maxi = 0;
    int len,nreaday;

    while(1){
        nreaday = poll (mypool, maxi+1, -1);
        if(nreaday <0){
            perror("poll accept error.\n");
            exit(1);
        }
        if(mypool[0].revents & POLLIN){
            connfd = accept(fd,NULL,NULL);
            if(connfd < 0){
                perror("accept error.\n");
                exit(1);
            }
            //监控到可读的connfd，把connfd放入数组
            for(i=1;i<MAX_FD;++i){
                if(mypool[i].fd < 0) {
                    mypool[i].fd  = connfd;
                    printf("read client %d connected\n", connfd);
                    break;
                }
            }
            if(i == MAX_FD){
                perror("too many connection.\n");
                exit(1);
            }
            mypool[i].events = POLLIN;
            if(i > maxi) maxi = i;

            if (--nreaday <= 0) continue;
        }
        for(i=1; i<=maxi ; ++i)
        {
            if((sockfd = mypool[i].fd) < 0)
                continue;
            /*该链接描述符实际发生的事件*/
            if(mypool[i].revents & POLLIN)
            {
                bzero(sockBuf,'\0');
                len = read(sockfd,sockBuf,BUF_LEN);
                //如果没有数据就等待
                if(len <= 0){
                    close(sockfd);              
                    mypool[i].fd = -1;
                    continue;
                }
                printf("client %d receive %s lenght is %d\n", sockfd, sockBuf,len);
                write(sockfd,sockBuf,len);
            }
        }
    }
    close(fd);
}
