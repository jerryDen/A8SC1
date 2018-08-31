
#ifndef __SERIAL_SERVER_H_
#define __SERIAL_SERVER_H_
/*功能：有效数据上报回调
 * 参数1：数据地址
 * 参数2：数据长度
 * 返回值：成功返回处理后数据的长度，失败返回负数
 * */
typedef int (*SerialRecvFunc)(unsigned char* ,unsigned int);
/*功能：解析数据,用于接收数据时调用
 * 参数1：源数据地址
 * 参数2：源数据长度
 * 参数3：目标数据地址(有效数据的地址)
 * 参数4：目标数据长度(有效数据的长度)
 * 返回值：成功返回被解析数据的总长度，失败返回负数
 * 注意:底层会通过此返回值来判断是否重复读取BUF，并调用次回调。
 * 如果长度&0x10000为正，底层将重复调用此回调
 * */
typedef int (*SerialParseFunc)(const unsigned char* , unsigned int,
								unsigned char* ,unsigned int *);
/*功能：构造数据，用于发送数据时调用
 * 参数1：源数据地址
 * 参数2：源数据长度
 * 参数3：目标数据地址(有效数据的地址)
 * 参数4：目标数据长度(有效数据的长度)
 * 返回值：成功返回构造数据的总长度
 * 注意:底层会通过此返回值来判断是否重复读取BUF，并调用次回调。
 * 如果长度&0x10000为正，底层将重复调用此回调
 *
 * ，失败返回负数
 * */
typedef int (*SerialBuildFunc)(const unsigned char* ,unsigned int,
								unsigned char* ,unsigned int);


#define SERIAL_READ_SYNC  1
#define SERIAL_READ_ASYNC 4
/*SerialOps：虚函数表
 * 一.setSerialHandle：设置异步处理函数
 *   参数1：串口服务函数操作集指针
 *   参数2：接收有效数据回调
 *   参数3：解析数据回调
 *   参数4：构造数据回调
 *   返回值:成功返回0,失败返回负数
 * 二.setBaudRate:设置波特率
 * 三.read:同步方式读函数
 * 	   参数1：串口服务函数操作集指针
 *   参数2：数据指针
 *   参数3：准备读取的数据长度
 *   参数4：超时时间(单位ms)
 *   返回值：成功返回读取的数据长度,失败：-1读取错误,-2超时.
 * 四.write：同步方式写函数
 * 五.切换读模式,mode:SERIAL_READ_SYNC 将异步切换成同步,mode:SERIAL_READ_ASYNC 将同步切换成异步。
 * */
typedef struct SerialOps{
	int (*setHandle)(struct SerialOps*,SerialRecvFunc,
		SerialParseFunc,SerialBuildFunc);
	int (*setBaudRate)(struct SerialOps* , int );
	int (*read)(struct SerialOps*,unsigned char *,int,int);
	int (*write)(struct SerialOps*,const unsigned char *,int);
	int (*changeReadMode)(struct SerialOps*,int );
}SerialOps,*pSerialOps;
/*
* createSerialServer：创建串口服务
* 	  参数1：设备节点绝对路劲
*    参数2：设置波特率
*    参数3：设置数据位
*    参数4：设置停止位
*    参数5：设置奇偶校验位
*    返回值：成功返回串口服务函数操作集指针,失败返回NULL
*/
pSerialOps createSerialServer(const char *devPath,int nBaudRate, int nDataBits, int nStopBits, int nParity);
/*
* destroySerialServer：销毁串口服务
*/
void destroySerialServer(pSerialOps * server);

#endif











