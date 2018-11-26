//
//  main.c
//  Client
//
//  Created by 林玮 on 2018/11/26.
//  Copyright © 2018年 林玮. All rights reserved.
//

#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<ctype.h>
#include<string.h>
#include<netdb.h>
#include<unistd.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<arpa/inet.h>
int Connect2Service(char*,char*);   //连接服务器
int client_main(int);
int main(int argc, const char * argv[]){
    int sockfd;
    if(argc!=3)
    {
        fprintf(stderr,"需要添加两个参数 -IP -Port\n");
        exit(1);
    }
    
    sockfd = Connect2Service(argv[1],argv[2]);
    client_main(sockfd);
    close(sockfd);
    printf("[+] 连接关闭\n");
    return 0;
    
}
int Connect2Service(char *IP,char *port){
    int sockfd,port_num;
    struct sockaddr_in server_addr;
    struct hostent *host;
    host = gethostbyname(IP);
    port_num = atoi(port);
    
    if((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1){
        fprintf(stderr, "Socket error:%s\a\n",strerror(errno));
        exit(1);
    }
    bzero(&server_addr,sizeof(struct sockaddr_in));
    server_addr.sin_family=AF_INET;        //协议
    server_addr.sin_port=htons(port_num);  //端口
    server_addr.sin_addr=*((struct in_addr*)host->h_addr);
    if(connect(sockfd, (struct sockaddr*)(&server_addr), sizeof(struct sockaddr)) == -1){
        perror("[!] 连接失败\n");
        exit(1);
    }
    return sockfd;
}
int client_main(int sockfd){
    char msg[20];
    printf("[+] PLz input msg:\n");
    scanf("%s",msg);
    send(sockfd, msg, 20, 0);
    sleep(15);
    close(sockfd);
    return 0;
}
