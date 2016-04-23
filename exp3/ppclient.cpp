#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>


#define ERR_EXIT(m) \
    do { \
        perror(m); \
        exit(EXIT_FAILURE); \
    } while (0)




int main(int argc,char* argv[])
{
    int sock,rec_len;
    struct sockaddr_in servaddr;
    char sendbuf[1024] = {0};
    char recvbuf[1024] = {0};
    char ipaddress[20];
    /*if(argc!=2)
    {
        printf("parameter wrong:which should be like this --ppclient 127.0.0.1 4567\n");
        exit(0);
    }*/


      if(argc!=4)
    {
        printf("parameter wrong:which should be like this --ppclient 127.0.0.1 4567\n");
        exit(0);
    }
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        //  listenfd = socket(AF_INET, SOCK_STREAM, 0)
        ERR_EXIT("socket error");



    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    //servaddr.sin_port = htons(4567);

    servaddr.sin_port=htons(atoi(argv[3]));

    //servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    /* inet_aton("127.0.0.1", &servaddr.sin_addr); */


    memset(ipaddress,0,sizeof(ipaddress));
    //sprintf(ipaddress,"%s",argv[1]);
    sprintf(ipaddress,"%s",argv[2]);
    if(inet_aton(ipaddress,&servaddr.sin_addr)==0)
    {
        printf("inet_aton error for :%s\n",ipaddress);
        exit(0);
    }

    if (connect(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        ERR_EXIT("connect error");

    printf("send msg to server:\n");

    
    while (fgets(sendbuf, sizeof(sendbuf), stdin) != NULL)
    {

        write(sock, sendbuf, strlen(sendbuf));
        read(sock, recvbuf, sizeof(recvbuf));

        //printf("Received msg:%s\n",recvbuf);
        fputs(recvbuf, stdout);

        memset(sendbuf, 0, sizeof(sendbuf));
        memset(recvbuf, 0, sizeof(recvbuf));
    }


    close(sock);


    return 0;
}
