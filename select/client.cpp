#include <iostream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define BUF_LEN 1023

void tserver(int argc, const char * argv[]);

int main(int argc, const char * argv[]) {
    tserver(argc,argv);
    return 0;
}
void tserver(int argc, const char * argv[]){
    std::cout << "t server" << std::endl;
    if(argc < 3){
        exit(-1);
    }
    
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    int backlog = atoi(argv[3]);
    
    std::cout << "ip=" << ip << " port="<<port << " backlog=" << backlog  << std::endl;
    
    int fd;
    int check_ret;
    
    fd = socket(PF_INET,SOCK_STREAM , 0);
    assert(fd >= 0);
    
    struct sockaddr_in address;
    bzero(&address,sizeof(address));
    
    //转换成网络地址
    address.sin_port = htons(port);
    address.sin_family = AF_INET;
    //地址转换
    inet_pton(AF_INET, ip, &address.sin_addr);
    check_ret = connect(fd, (struct sockaddr*) &address, sizeof(address));
    assert(check_ret >= 0);
    //发送数据
    //const char* oob_data = "abc";
    const char* normal_data = "my boy c++11 is a good language!";
    send(fd, normal_data, strlen(normal_data), 0);
    printf("send data len=%lu,msg=%s\n",strlen(normal_data),normal_data);

    //接受数据
    int len;
    char sockBuf[BUF_LEN];
    memset(sockBuf, '\0', BUF_LEN);
    len = recv(fd, sockBuf, BUF_LEN-1, 0);
    printf("receive data len=%d,msg=%s\n",len,sockBuf);

    close(fd);
}
