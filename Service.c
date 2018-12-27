//
//  main.c
//  operate
//
//  Created by 林玮 on 2018/11/26.
//  Copyright © 2018年 林玮. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/select.h>
#include <pthread.h>
#include <sys/syscall.h>
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"



void menu(void);
/* 多路复用 */
#define MYPORT 1234    // the port users will be connecting to
#define MAX_fd 5       // listen 监听的最大套接字数量
#define BUF_SIZE 200
int ammount = 0;                    // current ammount
void multiplex(void);


/* 多进程 */
#define service_port 1234 //端口
int initial_socket(void);       //多进程初始化socket
void multiprocess(void);
void Process(int);           //多进程处理demo


/* 多线程 */
#define port  1234
void multithreading(void);
void *thread_process(void *arg);


int main(int argc, const char * argv[]) {
    menu();
}

void menu(void){
    int mode = 0;
    printf("[+] Which mode do you want\n");
    puts("   [-]1. I/O复用\n   [-]2. 多进程\n   [-]3. 多线程\n");
    scanf("%d",&mode);
    switch (mode) {
        case 1:
            multiplex();
        case 2:
            multiprocess();
        case 3:
            multithreading();
    }
}




/* 多进程 */
void multiprocess(){
    int sockfd, newsockfd;               //sockfd = 监听端口      newsockfd = 处理信息
    struct  sockaddr_in remote_addr;
    socklen_t sin_size;
    sockfd = initial_socket();
    while(1){
        sin_size = sizeof(struct sockaddr_in);
        if((newsockfd = accept(sockfd, (struct sockaddr*)&remote_addr, &sin_size)) == -1){
            perror("[!] Accept 等待失败\n");
            exit(1);
        }
        printf(RESET"[+] 服务器收到 = %s\n",inet_ntoa(remote_addr.sin_addr));
        if(!fork()){
            close(sockfd);              //保证只有父进程使用sockfd
            Process(newsockfd);         //服务器端主函数
            close(newsockfd);           //子进程结束关闭fd
            exit(0);                    //结束子进程
            
        }
        close(newsockfd);               //保证只有子进程使用newsockfd
        
    }
}
int initial_socket(){
    int sockfd;
    int True = 1;
    struct sockaddr_in my_addr;
    if((sockfd=socket(AF_INET, SOCK_STREAM,0)) < 0){
        perror("[!] 套接字创建失败\n");
        exit(1);
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &True, sizeof(int)) == -1) {
        perror("[!] Setsockopt Error\n");
        exit(1);
    }
    my_addr.sin_family=AF_INET;
    my_addr.sin_port=htons(service_port);
    my_addr.sin_addr.s_addr=INADDR_ANY;
    bzero(&(my_addr.sin_zero),8);
    
    if(bind(sockfd,(struct sockaddr*)&my_addr,sizeof(struct sockaddr))==-1)
    {
        perror("[!] 绑定端口失败\n");
    }
    if(listen(sockfd,10) < 0)      //监听队列是10
    {
        perror("[!] Listening Error!\n");
        exit(1);
    }
    else{
        printf("[+] 开始监听端口-----------等待连接\n");
        return sockfd;
    }
}
void Process(int sockfd){
    char buf[BUFSIZ];
    memset(buf, '\0', sizeof(buf));
    recv(sockfd,buf,sizeof(buf),0);
    printf(BLUE"   [-] 收到数据 %s\n",buf);
    printf(BLUE"   [-] 当前进程 ID = %d\n",getpid());
}


/* 多线程 */
void multithreading(){
    int sockfd, newsockfd;               //sockfd = 监听端口      newsockfd = 处理信息
    pthread_t tid;
    struct  sockaddr_in remote_addr;
    socklen_t sin_size = sizeof(remote_addr);
    sockfd = initial_socket();
    while(1){
        if((newsockfd = accept(sockfd, (struct sockaddr *)&remote_addr, &sin_size)) == -1){
            perror("[!] Accept 等待失败\n");
            exit(1);
        }
        printf("[+] 服务器收到 = %s\n",inet_ntoa(remote_addr.sin_addr));
        if(newsockfd>0){
            pthread_create(&tid, NULL, thread_process, (void *)&newsockfd);
            pthread_detach(tid);
        }
    }
    
}
void *thread_process(void *arg){
    char buf[BUFSIZ];
    memset(buf, '\0', sizeof(buf));
    int sockfd = *(int *)arg;     //套接字
    int ret;
    ret = recv(sockfd,buf,sizeof(buf),0);
    //printf("%d\n",ret);
    printf(BLUE"   [-] 收到数据 %s\n",buf);
    close(sockfd);
    return 0;
}

/* 多路复用 */
void multiplex(){
    int fd_all[MAX_fd];                     //保存所有的fd
    int sock_fd, new_fd;                    // listen on sock_fd, new connection on new_fd
    struct sockaddr_in server_addr;         // server address information
    struct sockaddr_in client_addr;         // client address information
    socklen_t sin_size;
    int True = 1;
    char buf[BUF_SIZE];
    int ret;
    int i;
    /* timeout setting select函数的超时时间*/
    struct timeval timeout;
    struct timeval use;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    
    /*creat tcp socket*/
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror(BLUE"[!] Create Socket Error\n");
        exit(1);
    }
    
    /* 关闭等待时间，close后可立即使用 */
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &True, sizeof(int)) == -1) {
        perror(BLUE"[!] Setsockopt Error\n");
        exit(1);
    }
    
    /* config */
    server_addr.sin_family = AF_INET;         // host byte order
    server_addr.sin_port = htons(MYPORT);     // set port
    server_addr.sin_addr.s_addr = INADDR_ANY; // my IP
    //memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_ze6[ro));
    bzero(server_addr.sin_zero, sizeof(server_addr.sin_zero));   //保证大小
    
    /* bind */
    if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("[!] Bind Error");
        exit(1);
    }
    
    if (listen(sock_fd, MAX_fd) == -1) {
        perror("[!] listening Error\n");
        exit(1);
    }
    printf("[+] Listening on port %d\n",MYPORT);
    memset(&fd_all, 0, sizeof(fd_all)); //初始化fd数组
    fd_set  set;                        // 创建socket集
    fd_set  backup;
    int max = sock_fd;                  // select 处理的最大套接字 需要每轮更新
    sin_size = sizeof(client_addr);     //accept 参数
    FD_ZERO(&set);                   //集合初始化
    FD_ZERO(&backup);
    FD_SET(sock_fd, &set);
    
    
    while(1){
        /*每轮更新赋值，因为select之后会发生变化*/
        backup = set;
        use = timeout;
        
    
        /*select 操作*/
        ret = select(max + 1, &backup, NULL, NULL, &use);
        if (ret < 0) {
            perror("[!] Select Error \n");
            break;
        } else if (ret == 0) {
            //printf("[!] Timeout \n");
            continue;
        }
        
        /*是否有新的连接，有新连接相当于 '监听套接字' 可读*/
        if (FD_ISSET(sock_fd, &backup)){
            new_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &sin_size);
            if (new_fd < 0) {
                perror("[!] Accept New Error\n");
                continue;
            }
            printf(MAGENTA"[+] New Connection Client[%d] %s:%d\n", ammount,            //打印客户端的IP和端口
                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            /*遍历fd数组，将新的套接字加入fd_all*/
            for (i = 0; i < MAX_fd; i++){
                if (fd_all[i] != 0){
                    continue;
                }else{
                    fd_all[i] = new_fd;
                    printf(MAGENTA"[+] Add new to All_fd[%d]\n",i);
                    break;
                }
            }
            /* 将new_fd 加入fd_set（用于select）*/
            FD_SET(new_fd,&set);
            
            /*更新最大套接字*/
            if(max<new_fd){
                max = new_fd;
            }
            
         
        }

        /*轮询set中的fd，如果可用，进行读写*/
        for (i = 0; i < MAX_fd ; i++) {
            if (FD_ISSET(fd_all[i], &backup)){                    //判断当前的fd是否可读
                printf(GREEN"[+] FD->%d is ready \n",i);
                ret = recv(fd_all[i], buf, sizeof(buf), 0);
                if (ret <= 0) {
                    printf("   [-] Client[%d] Close\n",i);
                    close(fd_all[i]);                   //关闭当前fd
                    FD_CLR(fd_all[i], &set);            //同时清空set中的fd
                    fd_all[i] = 0;                      //初始化
                } else {        // receive data
                    if (ret < BUF_SIZE)
                        memset(&buf[ret], '\0', 1);      //添加结尾字符截断
                    printf("   [-] Recv Msg From Client[%d]:%s\n", i,buf);
                }
            }
        }

        printf(RED"[+] **********Socket_fd list***********\n");
        for (int i = 0; i < MAX_fd; i++) {
            printf(YELLOW"   [-] All_fd[%d]: %d\n",i,fd_all[i]);
        }
        puts(RED"[+] ************************************\n");
    }
    /* exit and close*/
    for (int i = 0; i < MAX_fd; i++) {
        if (fd_all[i] != 0){
            close(fd_all[i]);
        }
    }
    exit(0);
}
