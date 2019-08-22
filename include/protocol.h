#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#define FTP_ROOT_DIR   "/home/gec/tftp"


//命令号, 命令参数


//命令号->  整数
enum CMD_NO 
{
	FTP_CMD_LS = 1024, //
	FTP_CMD_GET,
	FTP_CMD_PUT,
	FTP_CMD_BYE,
	
	FTP_CMD_NUM // 命令个数
};

//出错码
enum resp_result
{
	RESP_SUCCESS = 0, //成功
	RESP_PACK_ERROR, //失败,包的长度不对
	RESP_PACK_NOEND, //包没有结束,可以再次收包
	RESP_PACK_NOFILE //没有可以获取的文件
};


//参数: 参数长度， 参数内容



/*
	0xc0 : 包头

	pkg_len ;//4bytes, 小端模式，整个数据包的长度
				 4(pkg_len) + 4 (cmd_no) + arg_1 +...
	cmd_no // 4bytes, 小端模式

	arg_1;
		arg_1_len; //4bytes , 小端模式
		arg_1_data; len长度
	arg_2:
		arg_2_len; //4bytes,小端模式
		arg_2_data;
	....

	0xC0:包尾
*/

// cmd: ls

//0xc0  ___包长度______    __命令号()__             0xc0
// 0xc0  0x80 0x00 0x00 0x00 


//cmd: get
//0xc0 ____包长度(4)___　___命令号(4)___   ___arg_1_Len(4)_   ___filename(r)___ 0xc0




#if 0
unsigned char cmd[1024];
int i = 0;

int pkg_len = 1024;

cmd[0] = 0xc0;

cmd[1] = pkg_len & 0xff;
cmd[2] = ( pkg_len >> 8) & 0xff;
cmd[3] = (pkg_len >> 16) & 0xff;
cmd[4] = (pkg_len >> 24) & 0xff;
#endif



//CMD_RESP
/*
	0xc0: 包头

	pkg_len: 整个数据包的长度，4bytes, 小端模式
	cmd_no :  命令号，4bytes, 小端模式，表示回复哪条命令
	resp_len: 回复内容的长度，4bytes, 小端模式
			result  + resp_conent
			1 + x
	result:  执行成功或失败，1 bytes , 0表示成功，其他表示失败
	resp_conent: 回复内容
			成功:
				ls: 文件名字  各文件名之间用空格分隔
				get: 文件大小，4bytes, 小端模式

			失败:
				 出错码

	0xc0:包尾
*/


//ls 的回复
// 0xc0  --pkg_Len(4)---   ---cmd_No(4)---  --resp_len(4)--  --result(1)--  --filenames(r, 名字以空格分开)--- 0xc0


//get 的回复
//0xc0  ---pkg_len(4)---  ----cmd_no(4)---  ---resp_len(4)---   ---result(1)--  --file_size(4, 小端模式)--　　0xc0

#endif
