#include "c.h"
//struct sockaddr_in sockaddr;
int my_userid;
char name[10];
int init()
{
	int conn;//connection
	c_init();//sockfd server(struct sockaddr_in)
	CHIS chis;
	memset(&chis,0,HSTR_LEN);
	//get global variable count from the last CHIS in the local file
	c_lseek(recordfd,-HSTR_LEN,SEEK_END);
	c_read(recordfd,&chis,HSTR_LEN);
	count=chis.count;

	conn=connect(sockfd,(struct sockaddr *)&server,sizeof(struct sockaddr));
	if(conn==-1)
	{
		perror("connect error:");
		exit(1);
	}
	//initialize usr_temp_p which indicates the status of a user
	bzero(&user_temp_p,USR_LEN);
	user_temp_p.status=0;

	printf("Initialization finished\n");
	return 0;
}

int login()
{
	PCK package;
	bzero(&package,PCK_LEN);
	memset(name,0,sizeof(name));
	printf("-------login>\n");
	printf("Please enter your User Name:\n>");
	scanf("%s",name);
	getchar();

	char password[15];
	memset(password,0,sizeof(password));
	printf("Please enter your password:\n>");
	scanf("%s",password);
	getchar();

	//put all the info into global USR usr and then set the PCK packid;
	//then call build_packet()
	bzero(&user_temp_b,USR_LEN);
	strncpy(user_temp_b.name,name,strlen(name));
	strncpy(user_temp_b.password,password,strlen(password));

	package.pack_id=1;//1--user

	build_packet(&package);
	//send the package 
	c_send(sockfd,&package,PCK_LEN,0);
	//codes below can be put in the child process
	c_recv(sockfd,&package,PCK_LEN,0);
	//parse the package

	int pack_type;
	USR usr;
	pack_type=parse_packet(&package);

	//if login success,status should be modified to 1
	if(pack_type==1&&user_temp_p.status==1)
	{
		printf(">>>>>>>>>>>>>>>login success\n");
		menu();
	}
	else
	{
		printf("login error\n");
		return 0;
	}

}
int regist()
{
	PCK package;
	char name[10];
	char password[15];
	memset(name,0,sizeof(name));
	memset(password,0,sizeof(password));

	printf("Welcome New User!\n");
	printf("Please enter a name(less than 10 characters):\n");
	scanf("%s",name);
	getchar();
	printf("Please enter the password(less than 15 characters):\n");
	scanf("%s",password);
	getchar();

	bzero(&user_temp_b,USR_LEN);
	bzero(&package,PCK_LEN);

	//put the info into the global USR
	strncpy(user_temp_b.name,name,strlen(name));
	strncpy(user_temp_b.password,password,strlen(password));

	//set pack_id(type)
	package.pack_id=1;//1----usr
	build_packet(&package);

	c_send(sockfd,&package,PCK_LEN,0);

	return 0;
}

//if send_msg is call 
//the usr_temp_p contains the usr info
//then get all the other data to build a packet
int send_msg()
{
	MSG msg;
	bzero(&msg,MSG_LEN);

	char text[TEXT_LEN];
	bzero(&text,sizeof(text));

	printf("Please input your message:\n>");
	scanf("%s",text);

	bzero(&msg_temp_b,MSG_LEN);
	strncpy(msg_temp_b.text,text,strlen(text));
	msg_temp_b.from=user_temp_p.userid;
	//msg_temp_b.to= this function is not yet provided.
	msg_temp_b.len=strlen(text);
	strncpy(msg_temp_b.remark,user_temp_p.name,strlen(user_temp_p.name));
	//finish filling the data in MSG
	//now set the type and build the packet
	msg.pack_id=2;//2-----msg
	build_packet(&package);
	//send to server
	c_send(sockfd,&package,PCK_LEN,0);

	return 0;
}

int load_history()
{
	//PCK package;
	CHIS chis;
	bzero(&chis,HSTR_LEN);
	bzero(&package);
	bzero(&chis_temp_b,HSTR_LEN);

	package.pack_id=3;//3-----chat_history
	
	c_lseek(recordfd,-HSTR_LEN,SEEK_END);
	c_read(recordfd,&chis,HSTR_LEN);

	chis_temp_b.count=chis.count;//set count=-1 as a request
	build_packet(&package);
	c_send(sockfd,&package,HSTR_LEN,0);

	return 0;
}
int menu()
{
	printf("--------------Welcome Back--------------\n");
	printf("Please select a command:\n");
	printf("H-------Chat History\n");
	printf("M-------Sending Messages\n");
	//printf("F-------Share Files\n");
	char c;
	E:c=getchar();
	getchar();
	switch(c)
	{
		case 'M':
		send_msg();
		break;

		case 'H':
		load_history();
		break;

		// case 'F':
		// share_files();
		// break;

		default:
		printf("wrong command,Please enter a command again!\n");
		goto E;
		break;
	}
	return 0;
}

void log_or_reg()
{
	char flag;
	printf("Please choose a command:\n", );
	printf("l-----login or r-----register\n");
	scanf("%c",&flag);
	getchar();
	switch(flag)
	{
		case 'l':
		login();
		break;

		case 'r':
		regist();
		break;

		default:
			printf("Error input\n");
			log_or_reg();
			break;
	}
	return 0;
}
int handle_usr()
{
	if(user_temp_p.userid!=0)
	{
		printf("Register success\n");
		printf("Switch to Login...\n");
		sleep(2);
		login();
	}
	else
	{
		printf("Register failed\n");
	}
	return 0;
}
int handle_msg()
{
	if(msg_temp_p.len==-1)
	{
		printf("Message sent!\n");
	}
	return 0;
}

int handle_chis()
{
	c_save_msg(&chis_temp_p);
	printf("\n");
	return 0;
}
int handle_package()
{
	PCK package;
	int pack_type;

	bzero(&package,PCK_LEN);
	c_recv(sockfd,&package,PCK_LEN,0);
	pack_type=parse_packet(&package);

	switch(pack_type)
	{
		case 1://usr
		handle_usr();
		break;

		case 2://msg
		handle_msg();
		break;

		case 3://chis
		handle_chis();
		break;

		default:
		printf("Bad package type occurred.\n");
		break;
	}
	return 0;
}


int main()
{
	pid_t pid;
	init();
	printf("--------------let's chat---------------\n");
	log_or_reg();
	pid=fork();
	if(pid==0)//child process
	{
		handle_package();
	}
	else if(pid>0)//parent process
	{
		menu();
	}
	else printf("fork error!\n");
}