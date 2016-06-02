#include "c.h"

int user_list_fd;//fd used to store user

int init()
{
	int c_init();

	user_list_fd=c_open("./user_list",O_RDWR|O_CREAT|0660);

	USER usr;
	memset(&usr,0,sizeof(USER));
	c_lseek(user_list_fd,0,SEEK_SET);
	c_write(user_list_fd,(char *)&usr,USR_LEN);
	
	int on = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
       perror("setsockopt error");

	c_bind(sockfd,(struct sockaddr*)&server,ADDR_LEN);
	listen(sockfd,7);//两个队列之和为7

	CHIS ch_his;

	bzero(&ch_his,HSTR_LEN);
	c_lseek(mainfd,0,SEEK_SET);
	c_write(mainfd,&ch_his,HSTR_LEN);
	c_lseek(mainfd,-HSTR_LEN,SEEK_END);
	c_read(mainfd,&ch_his,HSTR_LEN);
	count=ch_his.count;//initialize the value by the last count in the local file

	printf("initialization finished\n");
	return 0;
}


void menu()
{
	printf("===========Hello Admin===========\n");
	printf("Please select a command\n");
	printf("C------to look up chat history\n");
	printf("Q------to quit the programme\n");
	printf("input command>");
	char c;
	c=getchar();
	switch(c)
	{
		case 'C':
		   read_chat_history();
		   break;
		case 'E':
		   exit_sys();
		   break;
		default:
		   menu();
		   break;
	}

	getchar();
	return 0;
}
void package_parse(int conn)
{
	int rc;
	PCK package;
	int pack_type;
	while(1)
	{
		bzero(&package,PCK_LEN);
		rc=c_recv(sockfd,&package,PCK_LEN,0);
		if(rc>0)
			printf("package received\n");
		else if(rc==0)
		{
			printf("client closed\n");
			break;
		}
		else
		{
			perror("receive error:\n");
			exit(1);
		}

		pack_type=parse_package(&package);

		switch(pack_type)
		{
			case 1:
			user_transations();
			break;

			case 2:
			msg_transactions();
			break;

			case 3:
			chis_transations();
			break;

			default:
			break;

		}
	}
	
}
int handle_package()
{
	
	struct sockaddr_in addr_recv;
	pid_t pid;
	int size=ADDR_LEN;
	while(1)
	{
		if((conn=accept(sockfd,(struct sockaddr*)&addr_recv,&size))<0)
		{
			perror("accept error:");
			exit(1);
		}
		pid=fork();
		if(pid==-1)
		{
			perror("fork error");
			exit(1);
		}
		if(pid==0)
		{
			close(sockfd);
			package_parse(conn);
			exit(0);//wait a minute...
		}
		else close(conn);		
	}
	return 0;
}

int check_exist()
{ //i的遍历上线在user_list_fd所指文件的最后一个usr结构体的id
	USER usr;
	int i=0,count,exist;
	c_lseek(user_list_fd,-USR_LEN,SEEK_END);
	c_read(user_list_fd,&usr,USR_LEN);
	count=usr.userid;

	for(;i<=count;i++)
	{
		c_lseek(user_list_fd,i*USR_LEN,SEEK_SET);
		c_read(user_list_fd,&usr,USR_LEN);
		exist=strcmp(usr.name,user_temp_p.name);
		if(exist==0)//success
		{
			return 1;
			break;
		}
	}
	return 0;

}
int check_match()
{
	USER usr;
	int i=0,count,match;
	c_lseek(user_list_fd,-USR_LEN,SEEK_END);
	c_read(user_list_fd,&usr,USR_LEN);
	count=usr.userid;

	for(;i<=count;i++)
	{
		c_lseek(user_list_fd,i*USR_LEN,SEEK_SET);
		c_read(user_list_fd,&usr,USR_LEN);
		match=strcmp(usr.name,user_temp_p.name);
		if(match==0)//找到了
		{
			match=strcmp(usr.password,user_temp_p.password);
			if(match==0)
			{
				return 1;
				break;
			}
			else return 0;
		}
	}
	return 0;
}
int registration()
{
	USER usr;
	int id;
	c_lseek(user_list_fd,-USR_LEN,SEEK_END);
	c_read(user_list_fd,&usr,USR_LEN);
	id=usr.userid;
	const char *name;
	const char *password;
	memset(&usr,0,USR_LEN);
	name=&(user_temp_p.name);
	password=&(user_temp_p.password);
	strncpy(usr.name,name,10);
	strncpy(usr.password,password,15);
	usr.status=0;
	usr.userid= id+1;

	c_lseek(user_list_fd,0,SEEK_END);
	c_write(user_list_fd,&usr,USR_LEN);
	//返回一则成功信息。
	//c_send(sockfd,....)
}
int login()//return 1 success
{
	USER usr;
	int i=usr.userid;

	printf("a login request\n");

	c_lseek(user_list_fd,i*USR_LEN,SEEK_SET);
	c_read(user_list_fd,&usr,USR_LEN);
	int check;
	check= strcmp(usr.password,user_temp_p.password);
	if(check==0)//验证成功，将status置为1，写回文件。可以考虑建一个在线用户的数据结构；
	{
		usr.status=1;
		c_lseek(user_list_fd,-USR_LEN,SEEK_CUR);
		c_write(user_list_fd,&usr,USR_LEN);
		c_send(sockfd,(USER*)usr,USR_LEN,0);
		return 1;
	}
	else
	{
		printf("user:%d login error\n",usr.userid);
		usr.status=0;
		c_send(sockfd,(USER*)usr,USR_LEN,0);
        return 0;
	}
	
}
int user_transations()
{
	USER usr;
	memset(&usr,0,USR_LEN);
	int i=user_temp_p.userid;
	int is_exist;
	int is_match;
	//user结构体status最初被初始化为0，只有上线以后才会变为1
	//所以首先检查status的状态
	//如果为0，且用户名存在，则表示用户名密码错
	//如果为0，且用户名不存在，则表示用户要注册
	//如果为1，则表示用户想改密码
	if(user_temp_p.status==0)
	{
		
		is_exist=check_exist();
		if(is_exist==0)
			registration();
		else
			login();
	}
	else 
	{
		c_lseek(user_list_fd,i*USR_LEN,SEEK_SET);
		c_read(user_list_fd,&usr,USR_LEN);
		memset(usr.password,0,15);
		strncpy(usr.password,user_temp_p.password,strlen(user_temp_p.password));
		c_lseek(user_list_fd,i*USR_LEN,SEEK_SET);
		c_write(user_list_fd,&usr,USR_LEN);
		//modify password;
	}
}

int msg_transactions()
{
	c_save_msg(&msg_temp_p);
	printf("A new message coming!\n");
	printf("==<%s>%s\t\n",msg_temp_p.name,msg_temp_p.text);
	return 0;
}
int chis_transations()
{
	int i=chis_temp_p.count;
	int max;
	CHIS chis;
	memset(&chis,0,HSTR_LEN);
	c_lseek(recordfd,-HSTR_LEN,SEEK_END);
	c_read(recordfd,&chis,HSTR_LEN);
	max=chis.count;//get the count of all the history msg
	//send the message from i to the latest msg to cli
	for(i++;i<=max;i++)
	{
		c_lseek(recordfd,HSTR_LEN*i,SEEK_SET);
		c_read(recordfd,&chis,HSTR_LEN);
		c_send(conn,&chis,HSTR_LEN,0);//send to conn,conn is the socket for r/w
	}
	return 0;
}


int main()
{
	init();
	pid_t pid;
	pid=fork();
	if(pid==0)
	{
		handle_package();
	}
	else if(pid>0)
	{
		menu();
	}
	else
	{
		printf("ERROR!\n");
	}
	return 0;
}
