#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include "struct.h"
#include "ThreadPoll.h"

#define BUFFSIZE 1024

char res_welcome[]="220 linux ftp server\r\n";
char res_user[]="331 please insert password\r\n";
char res_pass[]="230 logined\r\n";
char res_user_err[]="430 Invalid username or password\r\n";
char res_notlogin[]="530 Not logged in\r\n";
char res_notimplement[]="502 Command not implemented\r\n";
char res_system[]="215 linux is the remote operating system\r\n";
char res_quit[]="221 Service closing control connection\r\n"; 
char res_directory[]="550 directory does not exist\r\n";
char res_port[]="200 Port command successful\r\n";
char res_transmission[]="150 Data connection already open transfer starting\r\n";
char res_transmission_complete[]="226 Closing data connection. Requested file action successful\r\n";
char res_type[]="200 Type set to I\r\n";

void * pre_action(void *);
void action(int , ThreadPoll *);
void prase_command(char * str , char * command , char * args);
void do_command(int fd , char * command , char * args , ThreadPoll * p);	
void command_user(int fd , char * args , ThreadPoll * p);
void command_pass(int fd , char * args , ThreadPoll * p);
void command_syst(int fd , char * args , ThreadPoll * p);
void command_quit(int fd , char * args , ThreadPoll * p);
void command_pwd (int fd , char * args , ThreadPoll * p);
void command_cwd (int fd , char * args , ThreadPoll * p);
void command_port(int fd , char * args , ThreadPoll * p);	
void command_list(int fd , char * args , ThreadPoll * p);
void command_type(int fd , char * args , ThreadPoll * p);
void command_retr(int fd , char * args , ThreadPoll * p);
void command_stor(int fd , char * args , ThreadPoll * p);

int listenfd;

COM com_com[]={
	{"USER",command_user},
	{"PASS",command_pass},
	{"SYST",command_syst},
	{"QUIT",command_quit},
	{"PWD" ,command_pwd},
	{"CWD" ,command_cwd},
	{"PORT",command_port},
	{"LIST",command_list},
	{"TYPE",command_type},
	{"RETR",command_retr},
	{"STOR",command_stor},
	{" ",NULL}
};


int main(int argc , char * argv[])
{

	struct sockaddr_in listensock , connectsock;
	int port;
	char work_dir[]="/home/bikli/project";
	if(chdir(work_dir)==-1)
	{
		printf("open work dir error\n");
		return 0;
	}
	printf("work dir = %s\n",get_current_dir_name());
	ThreadPoll threadpoll(10);
	threadpoll.add_key("user_success");
	threadpoll.add_key("pass_success");
	threadpoll.add_key("trans_addr");
	threadpoll.add_key("trans_port");
	threadpoll.add_key("dir");


	if(argc<2)
	{
		printf("usage server  <port>\n");
		return 0;
	}
	port = atoi(argv[argc-1]);
	printf("server port is %d\n",port);
	if((listenfd = socket(AF_INET,SOCK_STREAM,0))<0)
	{
		printf("listen socket errno %d\n",errno);
		return 0;

	}
	bzero(&listensock,sizeof(listensock));
	listensock.sin_family = AF_INET;
	listensock.sin_addr.s_addr=htonl(INADDR_ANY);
	listensock.sin_port=htons(port);
	if(bind(listenfd,(struct sockaddr *)&listensock,sizeof(listensock))<0)
	{
		printf("bind errno %d\n",errno);
	}
	if(listen(listenfd,8)<0)
	{
		printf("listen error %d\n",errno);
	}
	threadpoll.add_job(pre_action , nullptr , (void *)&threadpoll);	
	pause();	
	close(listenfd);
	threadpoll.destroy();
	return 0;	
}


void * pre_action(void * p)
{

	socklen_t connectsock_size; 
	struct sockaddr_in connectsock;
	ThreadPoll * threadpoll = (ThreadPoll *)p;
	int connectfd;

	int * user_success = (int *)malloc(sizeof(int));
	*user_success = 0;
	int * pass_success = (int *)malloc(sizeof(int));
	*pass_success = 0;
	char * trans_addr = (char *)malloc(sizeof(char)*16);
	int * trans_port = (int *)malloc(sizeof(int));
	*trans_port = 0;
	char * dir = (char *)malloc(sizeof(char)*255);
	strcpy(dir,"/home/bikli/project");	
	threadpoll->set_value("user_success" , (void *)user_success);
	threadpoll->set_value("pass_success" , (void *)pass_success);
	threadpoll->set_value("trans_addr" , (void *)trans_addr);
	threadpoll->set_value("trans_port" , (void *)trans_port);
	threadpoll->set_value("dir" , (void *)dir);

	while(1)
	{
		connectsock_size = sizeof(connectsock);
		threadpoll->lock();
		if((connectfd = accept(listenfd,(struct sockaddr *)(&connectsock),&connectsock_size))<0)			
		{
			threadpoll->unlock();
			if(errno == EINTR)
			{
				continue;
			}else
			{
				printf("accept error %d\n",errno);
				return 0;
			}
		}
		threadpoll->start();
		threadpoll->unlock();
		printf("one user\n");
		action(connectfd , threadpoll);
		close(connectfd);
		threadpoll->stop();
		printf("client disconnect\n");
	}
}

void action(int fd , ThreadPoll * p)
{
	char lp[5] , rp[BUFFSIZE];
	char command[BUFFSIZE];
	write(fd,res_welcome,strlen(res_welcome));	
	while(read(fd,command,BUFFSIZE))
	{
		prase_command(command , lp , rp);
		printf("\n");
		printf("command=%s args=%s lp=%d rp=%d\n",lp,rp,strlen(lp),strlen(rp));
		do_command(fd,lp,rp,p);	
	}


	return ;

}



void prase_command(char * str , char * command , char * args)
{
	sscanf(str,"%s %s",command,args);	
	return ;
}



void do_command(int fd , char * command , char * args , ThreadPoll * p)
{
	int i=0,num=0;
	while(com_com[i].funp!=NULL)
	{
		if(strcmp(com_com[i].command,command)==0)
		{
			printf("remote command is %s\n",command);
			int * user_success = (int *)p->get_value("user_success");
			if(*user_success==0)
			{
				if((strcmp(command,"USER")!=0)&&(strcmp(command,"PASS")!=0))
				{
					write(fd , res_notlogin , strlen(res_notlogin));		
					break;
				}
			}
			com_com[i].funp(fd , args , p);
			break;
		}
		i++;
	}
	if(i+1==sizeof(com_com)/sizeof(COM))
	{
		write(fd , res_notimplement , strlen(res_notimplement));		
	}
	return; 
}


void command_user(int fd , char * args , ThreadPoll * p)
{
	printf("command_user function\n");
	if(!strcmp(args,"bikli"))
	{
		write(fd , res_user , strlen(res_user));		
		int * user_success = (int *)p->get_value("user_success");
		*user_success=1;
	}
	else
	{
		write(fd , res_user_err , strlen(res_user_err));
		int * user_success = (int *)p->get_value("user_success");
		int * pass_success = (int *)p->get_value("pass_success");
		*user_success=0;
		*pass_success=0;
	}		
	return ;
}

void command_pass(int fd , char * args , ThreadPoll * p)
{
	printf("command_pass function\n");
	int * user_success = (int *)p->get_value("user_success");
	int * pass_success = (int *)p->get_value("pass_success");
	if(!(*user_success))
	{
		write(fd , res_notlogin , strlen(res_notlogin));		
	}
	else
	{
		if(!strcmp(args,"123"))
		{
			write(fd , res_pass , strlen(res_pass));		
			*pass_success=1;
		}
		else
		{
			write(fd , res_user_err , strlen(res_user_err));
			*user_success=0;
			*pass_success=0;
		}		
	}
	
	return ;

}



void command_syst(int fd , char * args , ThreadPoll * p)
{
	printf("command_syst function\n");
	write(fd , res_system , strlen(res_system));
	return ;
}

void command_quit(int fd , char * args , ThreadPoll * p)
{
	printf("command_quit function\n");
	int * user_success = (int *)p->get_value("user_success");
	int * pass_success = (int *)p->get_value("pass_success");
	*user_success=0;
	*pass_success=0;
	write(fd , res_quit , strlen(res_quit));
	return ;
}



void command_pwd (int fd , char * args , ThreadPoll * p)
{
	char * path = (char *)p->get_value("dir");
	char buf[255];
	printf("command_pwd function\n");
	//getcwd(path,255);
	sprintf(buf , "257 %s\r\n" , path);
	write(fd , buf , strlen(buf));
	return ;
}


void command_cwd(int fd , char * args , ThreadPoll * p)
{
	char * path = (char *)p->get_value("dir");
	char buf[255],tmp[255];
	bzero(tmp,255);
	strcpy(tmp,path);
	printf("command_cwd function\n");
	if(strcmp(args,"..")==0)
	{
		int num=0,i=0;
		while(path[i])
		{
			if(path[i]=='/')
			{
				num = i;
			}
			i++;
		}
		strncpy(buf , path , num);
		buf[num]='\0';
		printf(".. = %s\n",buf);
	}else if(strcmp(args,".")==0)
	{
		strcpy(buf,path);
	}else if(args[0]=='/')
	{
		sprintf(buf , "%s" , args);
	}else
	{
		sprintf(buf , "%s/%s" , path , args);
		printf("=====%s\n" , buf);
	}
	if(opendir(buf))
	{
		printf("change dir = %s\n",buf);
		//getcwd(path,255);
		bzero(path , 255);
		strcpy(path , buf);
		bzero(buf , 255);
		sprintf(buf , "257 %s\r\n" , path);
		write(fd , buf , strlen(buf));
	}
	else
	{
		write(fd , res_directory , strlen(res_directory));
		bzero(path,255);
		strcpy(path,tmp);
	}
}

void command_port(int fd , char * args , ThreadPoll * p)
{
	char * addr_1,* addr_2,* addr_3,* addr_4,* pp1,* pp2;
	int p1,p2;
	char * trans_addr = (char *)p->get_value("trans_addr");
	int * trans_port = (int *)p->get_value("trans_port");
	printf("command_port function ");
	write(fd , res_port , strlen(res_port));
	addr_1 = strtok(args,",");
	addr_2 = strtok(NULL,",");
	addr_3 = strtok(NULL,",");
	addr_4 = strtok(NULL,",");
	pp1 = strtok(NULL,",");
	pp2 = strtok(NULL,",");
	p1 = atoi(pp1);
	p2 = atoi(pp2);
	sprintf(trans_addr,"%s.%s.%s.%s",addr_1,addr_2,addr_3,addr_4);
	*trans_port = p1*256+p2;
	printf("ip=%s port=%d\n",trans_addr,*trans_port);
	return ;
}

void command_list(int fd , char * args , ThreadPoll * p)
{
	DIR * dir;
	struct dirent * dirent;
	struct stat st;
	char filebuf[255];
	char mode='-';
	unsigned long size=0;
	struct sockaddr_in clientaddr;
	socklen_t client_len;
	int clientfd;
	char * trans_addr = (char *)p->get_value("trans_addr");
	int * trans_port = (int *)p->get_value("trans_port");
	char * path = (char *)p->get_value("dir");
	
	printf("command_list function\n");
	write(fd , res_transmission , strlen(res_transmission));
	
	if((clientfd = socket(AF_INET,SOCK_STREAM,0))<0)
	{
		printf("create data socket error %d\n",errno);
	}
	bzero(&clientaddr,sizeof(clientaddr));
	clientaddr.sin_family=AF_INET;
	clientaddr.sin_port=htons(*trans_port);
	inet_pton(AF_INET,trans_addr,&clientaddr.sin_addr);
	client_len = sizeof(clientaddr);
	printf("data connecting\n");
	if(connect(clientfd,(struct sockaddr *)&clientaddr,client_len)<0)
	{
		printf("data connect error %d\n",errno);
		return;
	}

	//getcwd(path,255);
	printf("port path = %s\n",path);
	dir = opendir(path);
	char file_path[255];
	while(dirent=readdir(dir))
	{
		bzero(file_path,255);
		sprintf(file_path , "%s/%s" , path , dirent->d_name);	
		lstat(file_path , &st);
		if(S_ISDIR(st.st_mode))
		{
			mode='d';
		}
		else
		{
			mode='-';
		}
		sprintf(filebuf , "%c--------- 1 null null %d %s\r\n" , mode , st.st_size , dirent->d_name);
		write(clientfd , filebuf , strlen(filebuf));
	}
	write(fd , res_transmission_complete , strlen(res_transmission_complete));
	close(clientfd);
	printf("data connect is closed\n");
	return ;
}


void command_type(int fd , char * args , ThreadPoll * p)
{
	printf("command_type function\n");
	write(fd , res_type , strlen(res_type));
	return ;
}


void command_retr(int fd , char * args , ThreadPoll * p)
{
	char * buf[BUFFSIZE];
	FILE * file;
	size_t file_len;
	struct sockaddr_in clientaddr;
	socklen_t client_len;
	int clientfd;
	char * trans_addr = (char *)p->get_value("trans_addr");
	int * trans_port = (int *)p->get_value("trans_port");
	char * path = (char *)p->get_value("dir");

	printf("command_retr function\n");
	write(fd , res_transmission , strlen(res_transmission));
	if((clientfd = socket(AF_INET,SOCK_STREAM,0))<0)
	{
		printf("create data socket error %d\n",errno);
	}
	bzero(&clientaddr,sizeof(clientaddr));
	clientaddr.sin_family=AF_INET;
	clientaddr.sin_port=htons(*trans_port);
	inet_pton(AF_INET,trans_addr,&clientaddr.sin_addr);
	client_len = sizeof(clientaddr);
	printf("data connecting\n");
	if(connect(clientfd,(struct sockaddr *)&clientaddr,client_len)<0)
	{
		printf("data connect error %d\n",errno);
		return ;
	}
	char file_path[255];
	sprintf(file_path , "%s/%s" , path , args);
	file = fopen(file_path , "rb");	
	if(file)
	{
		while((file_len = fread(buf , sizeof(char) , BUFFSIZE , file))>0)
		{
			write(clientfd , buf , file_len);
			bzero(buf , BUFFSIZE);
		}
	}
	write(fd , res_transmission_complete , strlen(res_transmission_complete));
	close(clientfd);
	fclose(file);	
	printf("data connect is closed\n");	
	return ;
}


void command_stor(int fd , char * args , ThreadPoll * p)
{
	char * buf[BUFFSIZE];
	FILE * file;
	size_t getfile_len;
	struct sockaddr_in clientaddr;
	socklen_t client_len;
	int clientfd;
	char * trans_addr = (char *)p->get_value("trans_addr");
	int * trans_port = (int *)p->get_value("trans_port");
	char * path = (char *)p->get_value("dir");

	printf("command_stor function\n");
	write(fd , res_transmission , strlen(res_transmission));
	if((clientfd = socket(AF_INET,SOCK_STREAM,0))<0)
	{
		printf("create data socket error %d\n",errno);
	}
	bzero(&clientaddr,sizeof(clientaddr));
	clientaddr.sin_family=AF_INET;
	clientaddr.sin_port=htons(*trans_port);
	inet_pton(AF_INET,trans_addr,&clientaddr.sin_addr);
	client_len = sizeof(clientaddr);
	printf("data connecting\n");
	if(connect(clientfd,(struct sockaddr *)&clientaddr,client_len)<0)
	{
		printf("data connect error %d\n",errno);
		return ;
	}
	char file_path[255];
	sprintf(file_path , "%s/%s" , path , args);
	file = fopen(file_path , "wb");	
	if(file)
	{
		while(getfile_len = read(clientfd , buf , BUFFSIZE))
		{
			fwrite(buf , sizeof(char) , getfile_len , file);
			bzero(buf , BUFFSIZE);
		}
	}
	write(fd , res_transmission_complete , strlen(res_transmission_complete));
	close(clientfd);
	fclose(file);	
	printf("data connect is closed\n");	
	return ;

}
