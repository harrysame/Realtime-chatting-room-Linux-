#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <arpa/inet.h>

#define MAXLINE 1024
#define IPADDRESS "127.0.0.1"
#define SERV_PORT 8787

#define max(a,b) (a > b) ? a : b

static void handle_recv_msg(int sockfd, char *buf) 
{
	printf("client recv msg is:%s\n", buf);
	//fgets(buf, MAXLINE, stdin);
	scanf("%s",buf);getchar();
	//sleep(5);
	//buf[strlen(buf)]='\0';
	write(sockfd, buf, strlen(buf));
}

static void handle_connection(int sockfd)
{
	
	char sendline[MAXLINE],recvline[MAXLINE];
	int maxfdp,stdineof;
	fd_set readfds;
	int n;
	struct timeval tv;
	int retval = 0;
	while(1)
	{
		FD_ZERO(&readfds);
		FD_SET(sockfd,&readfds);
		maxfdp = sockfd;

		tv.tv_sec = 1000;
		tv.tv_usec = 0;

		retval = select(maxfdp+1,&readfds,NULL,NULL,&tv);
		//printf("retval=%d\n",retval);
		if (retval == -1) 
		{
			perror("select fail\n");
			return ;
		}

		if (retval == 0) {
		printf("client timeout.\n");
		continue;
		}

		if (FD_ISSET(sockfd, &readfds))
		{ 	
		//	printf("here\n");
			n = read(sockfd,recvline,MAXLINE);
			if (n <= 0)
			{
				fprintf(stderr,"client: server is closed.\n");
				close(sockfd);
				FD_CLR(sockfd,&readfds);
				return;
			}

			handle_recv_msg(sockfd, recvline);
		}
		else
		{
			printf("sockfd is not ready");
		}
	}
	//deleted line while（1)
}

int main(int argc,char *argv[])
{
	int sockfd;
	struct sockaddr_in servaddr;
	char sendbuf[1024]={0};
	char test[1024]={0};
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(sockfd==-1)
	{
		fprintf(stderr, "%s\n",strerror(errno) );
	}
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET,IPADDRESS,&servaddr.sin_addr);

	int retval = 0;
	retval = connect(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr));
	if (retval < 0) 
	{
		fprintf(stderr, "connect fail,error:%s\n", strerror(errno));
		return -1;
	}

	printf("client send to server .\n");
	
	printf("Please input your message: \n");

	//fgets(sendbuf, MAXLINE, stdin);
	
	//printf("%s is %d long\n",sendbuf,strlen(sendbuf));//检测函数
	scanf("%s",sendbuf);getchar();
	printf("the sendbuf is %dlong\n",strlen(sendbuf) );	
	sendbuf[strlen(sendbuf)]='\0';										

        if(write(sockfd, sendbuf, strlen(sendbuf))==-1)
        {
        	fprintf(stderr, "%s\n",strerror(errno) );
        	return -1;
        }
    
        handle_connection(sockfd);


	return 0;
}