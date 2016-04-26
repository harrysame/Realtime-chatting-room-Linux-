#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <sys/ioctl.h> 
#include <net/if.h>
#include <signal.h>


#define IPADDR      "127.0.0.1"
#define PORT        8787
#define MAXLINE     1024
#define LISTENQ     5
#define SIZE        7
#define MSG_SIZE    sizeof(struct MsgPackage)

typedef struct ClientManage
{
    int cli_cnt;        /*the totoal number of online clients*/
    int clifds[SIZE];   /*the array for storing the file_descriptor of the clients*/
    fd_set allfds;      /*the reading fd_set*/
    fd_set writefds;    /*the writing fd_set*/
    int maxfd;          /*the maximun value of the fd_set*/
}clientMng;

typedef struct MsgPackage 
{
    char seq[10];//the unique id of every msg
    char content[80];//message content
   char  timenow[80];//time_of_the_present
}myMsgPack;


static clientMng *svrCliMng = NULL;
static myMsgPack *msgRecord=NULL;
static int recordfd;//recevie the file descriptor of the chat history log
static int count;// indicator of the increasing unique number of msg
static int k=0;//indicator of the increasing number of msgRecord(struct)

/*===========================================================================
 * ==========================================================================*/
int init_files()
{
    recordfd=open("./chatlog.txt",O_RDWR|O_CREAT,0660);
    if(recordfd==-1)
    {
        perror("chatlog open failed");
        exit(1);
    }
    // lines above is added to achieve storage locally

    return 0;

}

static int create_server_proc(const char* ip,int port)
{
    int  fd;
    struct sockaddr_in servaddr;
    fd = socket(AF_INET, SOCK_STREAM,0);
    
    if (fd == -1) {
        fprintf(stderr, "create socket fail,erron:%d,reason:%s\n",
                errno, strerror(errno));
        return -1;
    }

    int reuse = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        return -1;
    }

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&servaddr.sin_addr);
    servaddr.sin_port = htons(port);

    if (bind(fd,(struct sockaddr*)&servaddr,sizeof(servaddr)) == -1) {
        perror("bind error: ");
        return -1;
    }

    listen(fd,LISTENQ);

    return fd;
}

static int accept_client_proc(int srvfd)
{
    struct sockaddr_in cliaddr;
    socklen_t cliaddrlen;
    cliaddrlen = sizeof(cliaddr);
    int clifd = -1;

    printf("accpet clint proc is called.\n");

ACCEPT:
        clifd=accept(srvfd,(struct sockaddr *)&cliaddr,&cliaddrlen);
        if(clifd==-1)
        {
            if(errno== EINTR)
            {
                goto ACCEPT;
                //continue;//continue will lead it go again to ACCEPT。
            }
            else
            {
                fprintf(stderr, "accept fail,error:%s\n",strerror(errno) );
                return -1;
            }
        }

        fprintf(stdout, "accept a new client:%s:%d\n",
        inet_ntoa(cliaddr.sin_addr),cliaddr.sin_port );

        //add the new file descriptor to the array
        int i=0;
        for(;i<SIZE;i++)
        {
            //printf("adding new clients\n");
            if(svrCliMng->clifds[i]<0)
            {
                svrCliMng->clifds[i]=clifd;
                svrCliMng->cli_cnt++;
                printf("new clients added,num=%d\n",svrCliMng->cli_cnt);

                break;
            }
        }

        if(i==SIZE)
        {
            fprintf(stderr, "Client Overflow.\n");
            return -1;
        }
        //linear array,not a looping one
   

}
int save_to_chat(myMsgPack *msg)
{
    //int n_block;
    printf("*****************saving to chatlog*************\n");
    printf("seq is %s\ncontent is:%s\ntime is:%s\n",msg->seq,msg->content,msg->timenow);
    // FILE *fp=fdopen(recordfd,"at+");
    // if(fp==NULL)
    // {
    //     perror("Open file failed");
    //     return -1;
    // }
    if(lseek(recordfd,0,SEEK_END)==-1)
        {
            perror("lseek error\n");
            return -1;
        }
    if(write(recordfd,msg,MSG_SIZE)==-1)
    {
        perror("write failed!\n");
        return -1;
    }
    //n_block=fwrite(&msg,MSG_SIZE,1,fp);
    //  printf("written:%d block\n",n_block);
    // if(n_block < 0)
    // {
    //  perror("write file error\n");
   //  return -1;
   // }//fdopen()won't switch it to a FILE pointer,used more often with poll（）
   return 0;
}

static int handle_client_msg(int fd, char *buf) 
{  
    char *num=(char *)malloc(10);
    assert(buf);
    printf("recv buf is :%s\n", buf);
    //write(fd, buf, strlen(buf));


    //lines below are newly added
    //form the message structure
    myMsgPack temp;
    memset(&temp,0,sizeof(myMsgPack));
    //temp.seq=++count;
    memset(num,0,10);
    sprintf(num,"%d",++count);
    strncpy(temp.seq,num,strlen(num));
    free(num);//remember to free num!!

    strncpy(temp.content,buf,strlen(buf));
    time_t now;
    time(&now);
    strncpy(temp.timenow,ctime(&now),strlen((ctime(&now))));
    save_to_chat(&temp);
    
    return count;
}


static void recv_client_msg(fd_set *readfds,fd_set *writefds)
{
    //printf("==============new message coming:====================\n");
    int i = 0, n = 0,j=0;
    int clifd,clifdw,rc;
    char buf[MAXLINE] = {0};
    int msgid;
    char tempwrite[MAXLINE]={0};
    for (i = 0;i <= svrCliMng->cli_cnt;i++) 
    {
        clifd = svrCliMng->clifds[i];
        if (clifd < 0) 
        {
            continue;
        }

        if (FD_ISSET(clifd, readfds)) 
        {
            //receive the msg from clients
            n = read(clifd, buf, MAXLINE);
            printf("message is:%s\n", buf);
            printf("length of message is:%d\n",n );
            if (n <= 0) 
            {
                FD_CLR(clifd, &svrCliMng->allfds);
                close(clifd);
                svrCliMng->clifds[i] = -1;
                continue;
            }
            msgid=handle_client_msg(clifd, buf);
            //give it a try

            for(j=0;j<= svrCliMng->cli_cnt;j++)//this part is originally creative
            {
                if(j==i)continue;
                clifdw=svrCliMng->clifds[j];
                if(clifdw<0)continue;

                if(FD_ISSET(clifdw,writefds))
                {
                printf("now sycronizing\n");
                //syncronize the msg to all the other clients online
                sprintf(tempwrite,"%d\t%s\n",msgid,buf);
                rc=write(clifdw,tempwrite,strlen(tempwrite));
                printf("tempwrite is:%s\n",tempwrite);
                if(rc>0)
                {
                    tempwrite[rc]='\0';
                    printf("sycronized!\n");
                }
                else
                {
                    perror("sycronize error!\n");
                }
                // sleep(5);
                }
             
            }
            
        }
                    

    }
}


static void handle_client_proc(int srvfd)
{

    int  clifd = -1;
    int  retval = 0;
    fd_set *readfds = &svrCliMng->allfds;
    fd_set *writefds= &svrCliMng->writefds;
    struct timeval tv;
    int i = 0;
    int tempcount=0;

    while (1) {
    	//reset the fd_set and time because both of them are modified by the kernel when events occur.
        
        /*add listening socket*/
        FD_ZERO(readfds);
        
        FD_SET(srvfd, readfds);

        FD_ZERO(writefds);
        FD_SET(srvfd,writefds);

        svrCliMng->maxfd = srvfd;

        tv.tv_sec = 1000;
        tv.tv_usec = 0;

        /*add client socket*/
        for (i = 0; i < svrCliMng->cli_cnt; i++) 
        {
            clifd = svrCliMng->clifds[i];
            FD_SET(clifd, readfds);//add the present client file descriptor to the readfd array
            FD_SET(clifd,writefds);//add the present client file descriptor to the writefd array 
            svrCliMng->maxfd = (clifd > svrCliMng->maxfd ? clifd : svrCliMng->maxfd);
            //select the max value of the file descriptor
        }

        retval = select(svrCliMng->maxfd + 1, readfds, writefds, NULL, &tv);

        if (retval == -1) {
            fprintf(stderr, "select error:%s.\n", strerror(errno));
            return;
        }

        if (retval == 0) {
            fprintf(stdout, "select is timeout.\n");
            continue;
        }

        if (FD_ISSET(srvfd, readfds)) 
        {
            /*listen to the request from the client*/
            accept_client_proc(srvfd);
        } 
        else 
        {
            /*receive the msg from client and respond to it*/
            recv_client_msg(readfds,writefds);
        }
    }
}

//server:free the memory
static void server_uninit()
{
    if (svrCliMng) {
        free(svrCliMng);
        svrCliMng = NULL;
    }
}
//server:allocate memory and initialize
static int server_init()
{
    svrCliMng = (clientMng *)malloc(sizeof(clientMng));
    //remember to check if it is null
    if (svrCliMng == NULL) {
        return -1;
    }
    
    memset(svrCliMng, 0, sizeof(clientMng));

    //the file descriptor value of the array is initialized with -1
    int i = 0;
    for (;i < SIZE; i++) {
        svrCliMng->clifds[i] = -1;
    }

    // if(chatlog exists)
    // {
    //     count=the last seq in the history log file
    // }
    // else count=0；

    return 0;
}

int main(int argc,char *argv[])
{
    init_files();
    int  srvfd;
    
    if (server_init() < 0) {
        return -1;
    }

    srvfd = create_server_proc(IPADDR, PORT);
    if (srvfd < 0) {
        fprintf(stderr, "socket create or bind fail.\n");
        server_uninit();
        return -1;
        }

    handle_client_proc(srvfd);

    return 0;

    
}