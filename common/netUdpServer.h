
#ifndef __SERIAL_SERVER_H_
#define __SERIAL_SERVER_H_
#include <netinet/in.h>
#include <sys/socket.h>

/*功能：有效数据上报回调
 * 参数1：数据地址
 * 参数2：数据长度
 * 返回值：成功返回处理后数据的长度，失败返回负数
 * */
typedef int (*UDPRecvFunc)(unsigned char* ,unsigned int);
/*功能：解析数据,用于接收数据时调用
 * 参数1：源数据地址
 * 参数2：源数据长度
 * 参数3：目标数据地址(有效数据的地址)
 * 参数4：目标数据长度(有效数据的长度)
 * 返回值：成功返回被解析数据的总长度，失败返回负数
 * */
typedef int (*UDPParseFunc)(const unsigned char* , unsigned int,
								unsigned char* ,unsigned int *);
/*功能：构造数据，用于发送数据时调用
 * 参数1：源数据地址
 * 参数2：源数据长度
 * 参数3：目标数据地址(有效数据的地址)
 * 参数4：目标数据长度(有效数据的长度)
 * 返回值：成功返回构造数据的总长度，失败返回负数
 * */
typedef int (*UDPBuildFunc)(const unsigned char* ,unsigned int,
								unsigned char* ,unsigned int);

/*UDPOps：虚函数表
 * 一.setHandle：设置异步处理函数
 *   参数1：服务函数操作集指针
 *   参数2：接收有效数据回调
 *   参数3：解析数据回调
 *   参数4：构造数据回调
 *   返回值:成功返回0,失败返回负数
 * 二.setBaudRate:设置波特率
 * 三.read:同步方式读函数
 * 	   参数1：服务函数操作集指针
 *   参数2：数据指针
 *   参数3：准备读取的数据长度
 *   参数4：超时时间(单位ms)
 *   返回值：成功返回读取的数据长度,失败：-1读取错误,-2超时.
 * 四.write：同步方式写函数
 * 	 参数1：服务函数操作集指针
 * 	 参数2：数据指针
 * 	 参数3：数据长度
 * 	 参数4：目的IP地址
 * 	 参数5：目的端口号
 * */
typedef struct UdpOps{
	int (*setHandle)(struct UdpOps*,UDPRecvFunc,
		UDPParseFunc,UDPBuildFunc);
	int (*read)(struct UdpOps*,unsigned char *,int,int);
	int (*ack)(struct UdpOps*,unsigned char *,int);
	int (*write)(struct UdpOps* , const char * , int ,
			uint32_t , int );
	int (*joinMulticast)(struct UdpOps*,unsigned int );
	int (*getRemoteInfo)(struct UdpOps*,struct sockaddr_in *);
	int (*setRemoteInfo)(struct UdpOps*,struct sockaddr_in );
	int (*setsockopt)(struct UdpOps* , int , int ,const void *, socklen_t);
}UdpOps,*pUdpOps;
/*
* createUdpServer：创建UDP服务
* 	  参数1：设备本机端口号
*    返回值：成功返回串口服务函数操作集指针,失败返回NULL
*/
pUdpOps createUdpServer(int netPort);
/*
* destroyUdpServer：销毁UDP服务
*/
void destroyUdpServer(pUdpOps * server);

#endif











