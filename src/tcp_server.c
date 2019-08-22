#include <sys/types.h>   
#include <sys/stat.h>       /* See NOTES */	
#include <fcntl.h>	   
#include <sys/socket.h>		   
#include <netinet/in.h>			   
#include <arpa/inet.h>
#include <linux/socket.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include "protocol.h"

#define FTP_DIR "/home/gec/tftp"

void send_pack(int connfd,unsigned char *cmd,int len)
{
	//
	printf("%d %x\n",connfd,cmd[0]);
	int ret = send(connfd,cmd,len,0);
	if(-1 == ret)
	{
		perror("send cmd error");
		return;
	}
	//
}

void send_error(int connfd)
{
	int i = 0;
	int pak_len = 4+4;
	unsigned char cmd[50] ={0};
	
	/**帧头**/
	cmd[i++] = 0xc0;
	
	/**包长**/
	cmd[i++] = pak_len & 0xff;
	cmd[i++] = pak_len>>8 & 0xff;
	cmd[i++] = pak_len>>16 & 0xff;
	cmd[i++] = pak_len>>24 & 0xff;

	/**错误码**/
	cmd[i++] = RESP_PACK_ERROR & 0xff;
	cmd[i++] = RESP_PACK_ERROR>>8 & 0xff;
	cmd[i++] = RESP_PACK_ERROR>>16 & 0xff;
	cmd[i++] = RESP_PACK_ERROR>>24 & 0xff;
	
	/**帧尾**/
	cmd[i++] = 0xc0;
	
	/**发送回复数据**/
	send_pack(connfd,cmd,i);
}

void repa_ls(int connfd)
{
	int i = 0;
	int pak_len = 0;
	unsigned char file[500] = {0};
	unsigned char cmd[500] = {0};
	DIR *dirp = opendir(FTP_DIR);
	if(NULL == dirp)
	{
		perror("open dir error");
		return ;
	}
	//
	struct dirent *dir;
	while((dir = readdir(dirp)) != NULL)
	{
		sprintf(file+strlen(file),"%s ",dir->d_name);
		printf("%d %s\n",strlen(file),dir->d_name);
	}
	//
	/**帧头**/
	cmd[i++] = 0xc0;
	
	/**包长**/
	pak_len = 4 + 4 + 4;
	cmd[i++] = pak_len & 0xff;
	cmd[i++] = pak_len>>8 & 0xff;
	cmd[i++] = pak_len>>16 & 0xff;
	cmd[i++] = pak_len>>24 & 0xff;

	/**错误码**/
	cmd[i++] = RESP_SUCCESS & 0xff;
	cmd[i++] = RESP_SUCCESS>>8 & 0xff;
	cmd[i++] = RESP_SUCCESS>>16 & 0xff;
	cmd[i++] = RESP_SUCCESS>>24 & 0xff;
	
	/**回复数据长度**/
	cmd[i++] = strlen(file) & 0xff;
	cmd[i++] = strlen(file)>>8 & 0xff;
	cmd[i++] = strlen(file)>>16 & 0xff;
	cmd[i++] = strlen(file)>>24 & 0xff;
	printf("recv ls data size %d\n",strlen(file));
	printf("%s\n",file);
	/**帧尾**/
	cmd[i++] = 0xc0;
	send_pack(connfd,cmd,i); //回复命令的处理结果
	
	send_pack(connfd,file,strlen(file));//回复命令数据
	//
}


void send_file_data(int connfd,int fd,int file_len)
{
	/*
		L E N S    E R R N   S I Z E
	*/
	
	unsigned char cmd[50] = {0};
	unsigned char buf[256] = {0};
	int pak_len;
	int size;
	int i;
	int ret = 0;
	while(1)
	{
		size = read(fd,buf,256);
		if(size == 0 || size==-1)
		{
			perror("read error");
			break;
		}
		pak_len = 4 + 4 + 4;
		ret += size;
		/*帧头*/
		i = 0;
		cmd[i++] = 0xc0;
		
		/*包长 小端模式*/
		cmd[i++] = pak_len & 0xff;
		cmd[i++] = pak_len>>8 & 0xff;
		cmd[i++] = pak_len>>16 & 0xff;
		cmd[i++] = pak_len>>24 & 0xff;
		
		/*回复码*/
		if(ret < file_len) //文件没完
		{
			cmd[i++] = RESP_PACK_NOEND & 0xff;
			cmd[i++] = RESP_PACK_NOEND>>8 & 0xff;
			cmd[i++] = RESP_PACK_NOEND>>16 & 0xff;
			cmd[i++] = RESP_PACK_NOEND>>24 & 0xff;
		}
		else
		{
			cmd[i++] = RESP_SUCCESS & 0xff;
			cmd[i++] = RESP_SUCCESS>>8 & 0xff;
			cmd[i++] = RESP_SUCCESS>>16 & 0xff;
			cmd[i++] = RESP_SUCCESS>>24 & 0xff;
		}
		
		/*正文长度*/
		cmd[i++] = size & 0xff;
		cmd[i++] = size>>8 & 0xff;
		cmd[i++] = size>>16 & 0xff;
		cmd[i++] = size>>24 & 0xff;
		
		/*帧尾*/
		cmd[i++] = 0xc0;
		
		send_pack(connfd,cmd,i); //回复数据包
		
		send(connfd,buf,size,0); //回复正文
	}
} 


void repa_get(int connfd,unsigned char *cmd)
{
/************************************解析get参数*******************************************************/
	int i = 8;
	int arg_len =  cmd[8] | cmd[9]<<8 | cmd[10]<<16 | cmd[11]<<24;
	char *file = (char *)malloc(arg_len+1);
	memset(file,0,arg_len+1);
	for(i=0;i<arg_len;i++)
	{
		file[i] = cmd[12+i]; //获取文件名
	}
	
/************************************打开文件读取数据并准备回复************************************************/
	int pak_len = 0;
	i=0;
	char ftp_file[100] = {0};
	sprintf(ftp_file,"%s/%s",FTP_DIR,file);
	int fd = open(ftp_file,O_RDONLY);
	if(-1 == fd)
	{
		perror("open error");
		
		/*帧头*/
		cmd[i++] = 0xc0;
		
		pak_len = 8;
		/*包长 小端模式*/
		cmd[i++] = pak_len & 0xff;
		cmd[i++] = pak_len>>8 & 0xff;
		cmd[i++] = pak_len>>16 & 0xff;
		cmd[i++] = pak_len>>24 & 0xff;
		
		/*回复码*/
		cmd[i++] = RESP_PACK_NOFILE & 0xff;
		cmd[i++] = RESP_PACK_NOFILE>>8 & 0xff;
		cmd[i++] = RESP_PACK_NOFILE>>16 & 0xff;
		cmd[i++] = RESP_PACK_NOFILE>>24 & 0xff;
		
		/*帧尾*/
		cmd[i++] = 0xc0;
		send_pack(connfd,cmd,i);
	}
	else
	{
		
		int file_len = lseek(fd,0,SEEK_END); //获取文件大小
		lseek(fd,0,SEEK_SET);
		send_file_data(connfd,fd,file_len); //回复文件内容
		
		close(fd);
	}
	
}


int recv_cmd(int connfd)
{
	int i=0;
	unsigned char ch = 0;
	unsigned char cmd[50] = {0};
	while(1)
	{
		//命令以0xc0开头,所以如果收到的不是0xc0则舍弃不要
		
		do
		{
			read(connfd,&ch,1);
		}while(ch != 0xc0);
		
		//排除连续的0xc0
		while(ch == 0xc0)
		{
			read(connfd,&ch,1);
		}//确保读取的是数据包的第一个字节
		i=0;
		
		while(ch != 0xc0)//读到的是数据包的内容,读到包尾结束
		{
			cmd[i++] = ch;
			read(connfd,&ch,1);
		}

		
		
		int cmd_len = cmd[0] | cmd[1]<<8 | cmd[2]<<16 | cmd[3]<<24;
		if(cmd_len != i)
		{
			printf("read cmd error,the pack len is no right\n");
			send_error(connfd);
			return 0;
		}
		
		int cmd_num = cmd[4] | cmd[5]<<8 | cmd[6]<<16 | cmd[7]<<24;
		switch(cmd_num)//判断收到的命令
		{
			case FTP_CMD_LS:
				repa_ls(connfd);
				break;
			case FTP_CMD_GET:			
				repa_get(connfd,cmd);
				break;
		}
		

	}
}



int main(int argc,char *argv[])
{
/**1.创建套接字**/
	int sockfd = socket(AF_INET,//IPv4协议族
						SOCK_STREAM,//流式套接字
						0//默认协议
						);
	if(-1 == sockfd)
	{
		perror("socket error");
		return -1;
	}
	/**设置套接字选项**/
	int val = 1;
	setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&val,4);//允许地址重用
	setsockopt(sockfd,SOL_SOCKET,SO_REUSEPORT,&val,4);//允许端口重用
/**2.绑定地址**/
	struct sockaddr_in sAddr;//以太网协议地址结构体 <--- 方便赋值
	memset(&sAddr,0,sizeof(sAddr));
	sAddr.sin_family = AF_INET;//协议族
	sAddr.sin_port = htons(atoi(argv[2]));//网络字节序的短整型
	sAddr.sin_addr.s_addr =inet_addr(argv[1]);//网络字节序的u32整数
	int ret;
	ret = bind(sockfd,//绑定的套接字
			(struct sockaddr*)&sAddr,//该套接字绑定的"地址"
			sizeof(sAddr)//地址长度
			);//将套接字与"地址"绑定
	if(-1 == ret)
	{
		perror("bind error");
		return -1;
	}
	
/**3.设置监听**/
	ret = listen(sockfd,10);
	if(-1 == ret)
	{
		perror("listen error");
		return -1;
	}

/**4.等待链接**/
	int connfd;
	struct sockaddr_in cAddr;
	socklen_t addrlen = sizeof(cAddr);
	while(1)
	{
		connfd = accept(sockfd,//套接字描述符,表示等待那个套接字上的链接
						(struct sockaddr*)&cAddr,//"地址"空间,用来保存客户端的地址
						&addrlen//空间,用来保存客户端地址长度
						);//如果有客户端链接,服务器端接受请求并返回链接描述符
						//链接描述符是用来与 对应的客户端进行通信
		if(-1 == connfd)
		{
			perror("accept error");
			return -1;
		}
		printf("connect client is:%s:%d\n",inet_ntoa(cAddr.sin_addr),ntohs(cAddr.sin_port));
			
		pid_t pid = fork();
		if(pid > 0)
		{
			close(connfd);
			continue;
		}
		else if(pid == 0)
		{
			//
			recv_cmd(connfd);
			exit(0);
		}
		
	}
/**7.处理并退出**/	
	close(sockfd);
	close(connfd);
	
	return 0;
}
