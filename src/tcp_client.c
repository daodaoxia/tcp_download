#include <sys/types.h>          /* See NOTES */	
#include <sys/stat.h>
#include <fcntl.h>	   
#include <sys/socket.h>	   
#include <netinet/in.h>			   
#include <arpa/inet.h>
#include <linux/socket.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "protocol.h"

int connect_server(char *ip,int port)
{
/**1.创建套接字**/
	int sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(-1 == sockfd)
	{
		perror("socket error");
		return -1;
	}
	
/**2.发起链接请求**/
	int ret;
	struct sockaddr_in sAddr;
	memset(&sAddr,0,sizeof(sAddr));
	sAddr.sin_family = AF_INET;
	sAddr.sin_port = htons(port);
	sAddr.sin_addr.s_addr = inet_addr(ip);
	ret = connect(sockfd,(struct sockaddr*)&sAddr,sizeof(sAddr));
	if(-1 ==ret)
	{
		perror("conncet error");
		return -1;
	}
	//
	return sockfd;
}

void send_cmd(int sockfd,unsigned char *cmd,int len)
{
	//
	int ret = send(sockfd,cmd,len,0);
	if(-1 == ret)
	{
		perror("send cmd error");
		return;
	}
	//
}

void recv_ls_val(int sockfd)
{
	/**
		服务器回复数据,事先会回复一个数据包
			0xc0  		 	L E N S   			 E R R N 			  S I Z E 			 0xc0 
						回复数据包长度		回复的验证信息			后续正文大小
		紧接着服务器会回复正文
			size个字节的数据
	**/
	
	int i=0;
	unsigned char ch = 0;
	unsigned char cmd[500] = {0};
/*************************************接收回复数据包*******************************************************/	
	//命令以0xc0开头,所以如果收到的不是0xc0则舍弃不要
	do
	{
		//
		read(sockfd,&ch,1);
		perror("read ls");
	}while(ch != 0xc0);
	//
	//排除连续的0xc0
	while(ch == 0xc0)
	{
		read(sockfd,&ch,1);
	}//确保读取的是数据包的第一个字节
	//
	while(ch != 0xc0)//读到的是数据包的内容,读到包尾结束
	{
		cmd[i++] = ch;
		read(sockfd,&ch,1);
	}
/***************************************数据包接收完成*****************************************************/	
	
	
/******************************************解析数据包******************************************************/	
	//解析包长
	int cmd_len = cmd[0] | cmd[1]<<8 | cmd[2]<<16 | cmd[3]<<24;
	if(cmd_len != i)
	{
		printf("read value error,the pack len is no right\n");
		//send_error(connfd);
		return;
	}
	
	//解析验证信息
	int err_no = cmd[4] | cmd[5]<<8 | cmd[6]<<16 | cmd[7]<<24;
	if(err_no == RESP_PACK_ERROR)
	{
		printf("cmd error\n");
		return;
	}
	
	int data_len = cmd[8] | cmd[9]<<8 | cmd[10]<<16 | cmd[11]<<24;
	printf("recv data len:%d\n",data_len);
/*****************************************解析完毕**********************************************************/	



/****************************************接收回复正文*******************************************************/
	memset(cmd,0,300);
	printf("data_len %d\n",data_len);
	recv(sockfd,cmd,data_len,0);
	printf("recv data is:\n%s",cmd);
	
}

void send_ls(int sockfd)
{
	/*
		ls 命令的数据包:
		0xc0  		 L E N S				C M D S        0xc0
				4个字节表示包的长度		4个字节表示命令
	*/
	int i = 0;
	int pak_len = 4+4;
	unsigned char cmd[50] ={0};
	
/**********************************组合数据包***************************************************/
	/**帧头**/
	cmd[i++] = 0xc0;
	
	/**包长**/
	cmd[i++] = pak_len & 0xff;
	cmd[i++] = pak_len>>8 & 0xff;
	cmd[i++] = pak_len>>16 & 0xff;
	cmd[i++] = pak_len>>24 & 0xff;

	/**命令**/
	cmd[i++] = FTP_CMD_LS & 0xff;
	cmd[i++] = FTP_CMD_LS>>8 & 0xff;
	cmd[i++] = FTP_CMD_LS>>16 & 0xff;
	cmd[i++] = FTP_CMD_LS>>24 & 0xff;
	
	/**帧尾**/
	cmd[i++] = 0xc0;
/******************************数据包组合完毕***************************************************/
	
	/**发送命令**/
	send_cmd(sockfd,cmd,i); //将数据包发送出去后,等待服务器的处理结果
	
	/**接收返回结果**/
	recv_ls_val(sockfd);

}


void recv_get_val(int sockfd,int fd)
{
	/**
		服务器回复数据,事先会回复一个数据包
			0xc0  		 	L E N S   			 E R R N 			  S I Z E 			 0xc0 
						回复数据包长度		回复的验证信息			后续正文大小
		紧接着服务器会回复正文
			size个字节的数据
	**/
	
	int i=0;
	unsigned char ch = 0;
	unsigned char cmd[300] = {0};
/*************************************接收回复数据包*******************************************************/	
	//命令以0xc0开头,所以如果收到的不是0xc0则舍弃不要
	do
	{
		//
		read(sockfd,&ch,1);
	}while(ch != 0xc0);
	
	//排除连续的0xc0
	while(ch == 0xc0)
	{
		read(sockfd,&ch,1);
	}//确保读取的是数据包的第一个字节
	
	while(ch != 0xc0)//读到的是数据包的内容,读到包尾结束
	{
		cmd[i++] = ch;
		read(sockfd,&ch,1);
	}
/***************************************数据包接收完成*****************************************************/	
	
	
/******************************************解析数据包******************************************************/	
	//解析包长
	int cmd_len = cmd[0] | cmd[1]<<8 | cmd[2]<<16 | cmd[3]<<24;
	if(cmd_len != i)
	{
		printf("read value error,the pack len is no right\n");
		//send_error(connfd);
		return;
	}
	
	//解析验证信息
	int err_no = cmd[4] | cmd[5]<<8 | cmd[6]<<16 | cmd[7]<<24;
	if(err_no == RESP_PACK_ERROR)
	{
		printf("cmd error\n");
		return;
	}
	
	int data_len = cmd[8] | cmd[9]<<8 | cmd[10]<<16 | cmd[11]<<24;
	printf("recv data len:%d\n",data_len);
/*****************************************解析完毕**********************************************************/	
	
/*****************************************接收文件数据******************************************************/
	unsigned char *p = (unsigned char *)malloc(data_len+1);
	int ret=0;
	while(1)
	{
		
		ret += read(sockfd,p+ret,data_len);
		if(ret == data_len)
		{
			break;
		}
		else if(ret == -1 || ret == 0)
		{
			perror("read error");
			return;
		}
		
	}
	
	write(fd,p,data_len);//将正文写入本地文件
	
/********************************************判断文件是否发送完成*******************************************************/
	if(err_no == RESP_PACK_NOEND) //文件内容没有发完,会在后续发送
	{
		
		recv_get_val(sockfd,fd); //再次接收后续包
	}
}


void send_get(int sockfd,char *file)
{
	/**
		get命令包格式:
			0xc0     L E N S      C M D S   	    S I Z E   			A R G S . . . .			0xc0
					包长(4bytes)  命令号(6bytes)    参数长度(4bytes)	参数正文(size bytes)
	**/
	int arg_len = strlen(file);
	int pak_len = 		4	+		4	+				4	+ 			arg_len;
	unsigned char cmd[50] = {0}; //保存命令数据包
	int i=0;
/****************************************封装命令包****************************************************/
	/*包头*/
	cmd[i++] = 0Xc0;
	
	/*包长 小端模式*/
	cmd[i++] = pak_len & 0xff;
	cmd[i++] = pak_len>>8 & 0xff;
	cmd[i++] = pak_len>>16 & 0xff;
	cmd[i++] = pak_len>>24 & 0xff;
	
	/*命令号  小端模式*/
	cmd[i++] = FTP_CMD_GET & 0xff;
	cmd[i++] = FTP_CMD_GET>>8 & 0xff;
	cmd[i++] = FTP_CMD_GET>>16 & 0xff;
	cmd[i++] = FTP_CMD_GET>>24 & 0xff;
	
	/*参数长度*/
	cmd[i++] = arg_len & 0xff;
	cmd[i++] = arg_len>>8 & 0xff;
	cmd[i++] = arg_len>>16 & 0xff;
	cmd[i++] = arg_len>>24 & 0xff;
	
	/*参数正文*/
	int j=0;
	for(j=0;j<arg_len;j++)
	{
		cmd[i++] = file[j];
	}
	
	/*包尾*/
	cmd[i++] = 0xc0;
/********************************************封包完成****************************************************/	
	
	/*发送命令*/
	send_cmd(sockfd,cmd,i);
	
	/*接收回复*/
	int fd = open(file,O_RDWR|O_CREAT|O_TRUNC,0777);
	if(-1 == fd)
	{
		perror("open error");
		return;
	}
	
	recv_get_val(sockfd,fd);
	
	close(fd);
}

int main(int argc,char *argv[])
{

	int sockfd = connect_server(argv[1],atoi(argv[2]));
	if(-1 == sockfd)
	{
		return -1;
	}

	unsigned char cmd[5] = {0};
	unsigned char file[20] = {0};
	while(1)
	{
		memset(cmd,0,5);
		scanf("%s",cmd);
				// ls			<--- 查看服务器文件列表
				// get file		<--- 从服务器获取文件列表
				// put file		<--- 上传文件到服务器
				// bye			<--- 告知服务器断开链接
		if( strcmp(cmd,"ls") == 0 )
		{
			//
			send_ls(sockfd);
		}
		else if(strcmp(cmd,"get")==0) // get 1.txt
		{
			scanf("%s",file);
			send_get(sockfd,file);
		}
	}
	
	/**5.处理并退出**/
	close(sockfd);
	
	return 0;
}