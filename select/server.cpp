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

    //读的fd_set
    fd_set  readset,writeset; 
    int  sockArray[MAX_FD];
    int i,j,len,n;
    int connfd;
    for(i=0;i<MAX_FD;++i){
        sockArray[i] = -1;
    }

    int dealFd;
    //不要每次都全部循环一遍
    int maxfd,maxi;
    maxfd = fd+1; 
    maxi = -1;

    while(1){
        //读句柄,清零
        FD_ZERO(&readset);
        FD_ZERO(&writeset);
        //把当前的socket加入监控
        FD_SET(fd,&readset);
        FD_SET(fd,&writeset);
        //初始化数据
        bzero(sockBuf,'\0');
        //select监视的文件句柄数，视进程中打开的文件数而定
        //一般设为你要监视各文件
        if (select(maxfd+1, &readset, NULL, NULL, NULL) < 0)
        {
            continue;
        }

        connfd = accept(fd,NULL,NULL);

        //将可读放入就绪里面
        if(FD_ISSET(fd,&writeset)){
            //监控到可读的connfd，把connfd放入数组
            for(i=0;i<MAX_FD;++i){
                if(sockArray[i] < 0) {
                    sockArray[i] = connfd;
                    printf("write client %d connected\n", connfd);
                    break;
                }
            }
            //fd用完
            if(i == MAX_FD){
                perror("too many connection.\n");
                exit(1);
            }
            //如果出现可写，加入监控
            FD_SET(connfd,&writeset);
            if(connfd > maxfd) maxfd = connfd;
            if(i > maxi) maxi = i;
        }

        //将可读放入就绪里面
        if(FD_ISSET(fd,&readset)){
            //监控到可读的connfd，把connfd放入数组
            for(i=0;i<MAX_FD;++i){
                if(sockArray[i] < 0) {
                    sockArray[i] = connfd;
                    printf("read client %d connected\n", connfd);
                    break;
                }
            }
            //fd用完
            if(i == MAX_FD){
                perror("too many connection.\n");
                exit(1);
            }
            //如果出现可读，加入监控
            FD_SET(connfd,&readset);
            if(i > maxi) maxi = i;
            printf("i=%d\n", i);
        }
        //集中处理读写
        for(i=0;i<=maxi;++i){
            dealFd = sockArray[i];
            if(dealFd < 0) continue;
            if(dealFd > 0)printf("%d,", dealFd);
            if(FD_ISSET(dealFd, &readset)){
                bzero(sockBuf,'\0');

                len = read(dealFd,sockBuf,BUF_LEN);
                //如果没有数据就等待
                if(len <= 0){
                    close(dealFd);
                    FD_CLR(dealFd , &readset);
                    sockArray[i] = -1;
                    break;
                }
                FD_CLR(dealFd , &readset);
            }
            if(FD_ISSET(dealFd, &writeset)){
                printf("client %d send %s lenght is %d\n", dealFd, sockBuf,len);
                write(dealFd,sockBuf,len);
                FD_CLR(dealFd , &writeset);
            }
            //此处不用关闭，因为在开头我们会统一处理
            sockArray[i] = -1;
        }
    }
    close(fd);
}
