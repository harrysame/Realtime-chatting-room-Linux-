#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h>
#define Maxlen 1024
#define ERR_EXIT(m) \
    do { \
        perror(m); \
        exit(EXIT_FAILURE); \
    } while (0)

void do_service(int);

int main(void)
{
    long maxlen;
    signal(SIGCHLD, SIG_IGN);//deal with the zombie process
    int listenfd; //used for listening
    if ((listenfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        //  listenfd = socket(AF_INET, SOCK_STREAM, 0)
        ERR_EXIT("socket error");

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;//Internet Address cluster
    servaddr.sin_port = htons(4567);//Port Number
    //servaddr.sin_addr.s_addr = htonl(INADDR_ANY);//IP address
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    /* inet_aton("127.0.0.1", &servaddr.sin_addr); */

    int on = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
        ERR_EXIT("setsockopt error");

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        ERR_EXIT("bind error");

    if (listen(listenfd, SOMAXCONN) < 0) 
        ERR_EXIT("listen error");

    struct sockaddr_in peeraddr; //construct the struct for sockaddr_in
    socklen_t peerlen = sizeof(peeraddr); //needs to be initialized
    int conn; // connected sockfd

    pid_t pid;

    while (1)
    {
        if ((conn = accept(listenfd, (struct sockaddr *)&peeraddr, &peerlen)) < 0)
            ERR_EXIT("accept error");
        printf("recv connect ip=%s port=%d\n", inet_ntoa(peeraddr.sin_addr),
               ntohs(peeraddr.sin_port));

        pid = fork();
        if (pid == -1)
            ERR_EXIT("fork error");
        if (pid == 0)
        {
            // child process
            close(listenfd);
            do_service(conn);
            exit(EXIT_SUCCESS);
        }
        else
            close(conn); //parent process
    }

    return 0;
}

void do_service(int conn)
{
    char recvbuf[1024];
    long maxlen;
    while (1)
    {
        memset(recvbuf, 0, sizeof(recvbuf));
        int ret = read(conn, recvbuf, sizeof(recvbuf));
        if (ret == 0)   //client is closed
        {
            printf("client close\n");
            break;
        }
        else if (ret == -1)
            ERR_EXIT("read error");
        fputs(recvbuf, stdout);
        //printf("msg received from client:%s\n",recvbuf );
        write(conn, "msg received\n", 1024);
    }
}