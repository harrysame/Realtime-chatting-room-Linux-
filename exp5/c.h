#include <math.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <sys/ioctl.h> 
#include <net/if.h>
#include <signal.h>
#include <ncurses.h>
#include <math.h>

#define SEVR_IP     "127.0.0.1"
#define SEVR_PORT   8787
#define CNTNT_LEN   150
#define PRT_LEN     10
#define TEXT_LEN    30
#define USR_LEN     sizeof(struct usr_packet)

#define MSG_LEN     sizeof(struct message_packet)

#define HSTR_LEN    sizeof(struct chhis_packet)

#define PCK_LEN		sizeof(struct pck)

#define ADDR_LEN    sizeof(struct sockaddr_in)
//find ADDR_LEN to see if there is anything wrong

typedef struct usr_packet
{
	int userid;
	int status;//1--online 0--offline
	char name[10];
	char password[15];
}USER;

typedef struct message_packet
{
	char text[TEXT_LEN];
	int from;
	int to;
	int len;//real length of the text msg
	char remark[10];//extra info if necessary
}MSG;

typedef struct chhis_packet
{
	char text[TEXT_LEN];
	char time[25];
	int from;
	int to;
	int count;
}CHIS;

typedef struct pck
{
	int pack_id;
	union 
	{
		struct usr_packet;
		struct message_packet;
		struct chhis_packet;
	}data;
}PCK;

//define global variables
int recordfd;//the chathistory local file discriptor
int sockfd,listenfd,conn;
int count;//the sequence number of the chat record
struct sockaddr_in server;

USER user_temp_b,user_temp_p;
CHIS chis_temp_b,chis_temp_p;
MSG  msg_temp_b,msg_temp_p;
PCK *pck;
//peripherial functions
int c_init();
char *c_get_time();
int c_clean_stdin();
int c_print(char *p_msg,int size);
int c_input(char *g_msg);
int c_socket(int domain,int type,int protocol);
int c_bind(int fd,const struct sockaddr *addr,int addrlen);
int c_recv(int fd,void * buf,size_t len,int flags);
int c_send(int fd,void *buf,size_t len,int flags);
int c_open(const char* pathname,int flags);
int c_read(int fd,void *msg,int len);
int c_write(int fd, void *msg, int len);
int c_lseek(int fd,off_t size,int position);
int c_save_msg(MSG *pmsg);
int build_packet(PCK *package);
int parse_packet(PCK *package);
int get_page_size();
int read_chat_history();
int exit_sys();
//
char *c_get_time()
{
	time_t timenow;
	time(&timenow);
	return (ctime(&timenow));
}

int c_clean_stdin()
{
	while('\n'==getchar())
	{
		continue;
	}
	return 0;
}

int c_print(char *p_msg,int size)//print_msg
{
	int i=1;
	for(;i<=size;i++)
	{
		if(*p_msg!='\n')
		{
			printf("%c",*p_msg);
			p_msg++;
		}
		else return 0;
	}
}

int c_input(char *g_msg)//get_msg
{
	char c='\0';
	int i;

	for(i=0;i<TEXT_LEN;i++)
	{
		g_msg[i]=getchar();
		if(g_msg[i]=='\n')
			return 0;
	}
	printf("MAXLEN of text reached!\n");
	return 0;
}

int c_socket(int domain,int type,int protocol)
{
	int fd;
	if((fd=socket(domain,type,protocol))==-1)
	{
		perror("socket failed:");
		exit(1);
	}
	return fd;
}

int c_bind(int fd,const struct sockaddr *addr,int addrlen)
{
	if(bind(fd,addr,addrlen)==-1)
	{
		perror("bind err:");
		exit(1);
	}
	return (0);
}

int c_recv(int fd,void * buf,size_t len,int flags)
{
	if(-1==recv(fd,buf,len,flags))
	{
		perror("recv error:");
		exit(1);
	}
	return 0;
}

int c_send(int fd,void *buf,size_t len,int flags)
{
	if(-1== send(fd,buf,len,flags))
	{
		perror("send error:");
		exit(1);
	}
	return 0;
}

int c_open(const char* pathname,int flags)
{
	int fd;
	if((fd=open(pathname,flags))==-1)
	{
		perror("open failed:");
		exit(1);
	}
	return fd;
}

int c_read(int fd,void *msg,int len)
{
	int rc;//the bytes that have been read
	if((rc=read(fd,msg,len))==-1)
	{
		perror("read error:");
		exit(1);
	}
	return rc;
}

int c_write(int fd, void *msg, int len)
{
	int wc;//the bytes that have been written
	if((wc=write(fd,msg,len))==-1)
	{
		perror("write error:");
		exit(1);
	}
	return(wc);
}

int c_init()
{
	CHIS chis;
	memset(&chis,0,HSTR_LEN);
	recordfd=c_open("./chat_log",O_RDWR|O_CREAT|0660);
	c_lseek(recordfd,0,SEEK_SET);
	c_write(recordfd,&chis,HSTR_LEN);
	sockfd=c_socket(AF_INET,SOCK_STREAM,0);

	bzero(&server,sizeof(server));
	server.sin_family=AF_INET;
	inet_pton(AF_INET,"127.0.0.1",&server.sin_addr);
	server.sin_port=htons(SEVR_PORT);

	perror("init:");
	return 0;

}

int c_lseek(int fd,off_t size,int position)
{
	if(-1== lseek(fd,size,position))
	{
		perror("seek error:");
		exit(1);
	}
	return 0;
}

int  c_save_msg(struct message_packet *pmsg)
{
	CHIS hstr;
	bzero(&hstr,HSTR_LEN);
	count++;
	hstr.from=pmsg->from;
	hstr.to=pmsg->to;

	strncpy(hstr.text,pmsg->text,TEXT_LEN);
	strncpy(hstr.time,c_get_time(),25);

	c_lseek(recordfd,0,SEEK_END);
	c_write(recordfd,&hstr,HSTR_LEN);

	return 0;
}
//before build_packet process,other function hasd prepared data
//in global variable user_temp_b,chis_temp_b,msg_temp_b

int build_packet(PCK *package)
{
	switch(package->pack_id)
	{
		case 1:
			package->data=user_temp_b;
		break;

		case 2:
			package->data=msg_temp_b;
		break;

		case 3:
			package->data=chis_temp_b;
		break;

		default:
		break; 
	}
	return 0;
}

//parse_packet is going to unpack the package 
//and put data into different kinds of global variable(struct) 
//such as user_temp_p,chis_temp_p,msg_temp_p
//to let other functions to process further
int parse_packet(PCK *package)
{
	switch(package->pack_id)
	{
		case 1:
		user_temp_p=package->data;
		break;
		case 2:
		msg_temp_p=package->data;
		break;
		case 3:
		chis_temp_p=package->data;
		break;
		default:
		break;
	}
	return package->pack_id;
}
int get_page_size()
{
	CHIS page_size_reader;
	
	c_lseek(mainfd, -HSTR_LEN, SEEK_END);
	c_read(mainfd, &page_size_reader, HSTR_LEN);

	return(page_size_reader.count);
}
int read_chat_history()
{
	printf("-----------chat history----------\n");
	printf("(n-next page;p-previous page;q-quit)\n" );
	int page_num;
	int remains;
	int berry=get_page_size();

	page_num=berry/PTR_LEN;
	remains=berry%PTR_LEN;

	if(remains!=0)
		page_num++;
	printf("There are %d page and %d total items\n",page_num,berry );
	int i=-1;
	while(1)
	{
		char flag;
		if((berry+i*PTR_LEN)>=0)
		{
			printf("(%s~%d)\n",(berry+i*PTR_LEN),(berry+(i+1)*PTR_LEN);
			print_history(PTR_LEN,i);
			printf("##################################\n");
			while('\n'==(flag = getchar()))
			{}
			switch(flag)
			{
				case 'p':
				i--;
				break;

				case 'n':
				i++;
				break;

				case 'q':
				return 0;

				default:
				break;
			}
			if(i>=0)
			{
				printf("The end!\n");
				printf("Return to menu!\n");
			}

		}
		else
		{
			printf("(1~%d)\n",remains);
			print_history(remains,0);
			printf("-----------chat history over----------\n");
			return 0;

		}
	}
	return 0;

}
int exit_sys()
{
	close(sockfd);
	close(recordfd);
	kill(0,SIGABRT);
	exit(0);
}







