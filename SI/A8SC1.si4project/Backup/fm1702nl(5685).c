#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "icCard/fm1702nl.h"
#include "serial/serialServer.h"
#include "common/Utils.h"
#include "common/debugLog.h"

typedef struct {
	unsigned char AckType;
	//有数据时，是数据的开始，无数据时，是效验
	unsigned char DataStart;
} T_READER_HEAD, *pT_READER_HEAD;


#define READER_MTU	(128 + sizeof(T_READER_HEAD))
typedef struct {
	IcDoorCardOps ops;
	pSerialOps serialOps;

} IcDoorCardServer, *pIcDoorCardServer;
static unsigned char changeEsc(unsigned char escChar);
static DoorCardRecvFunc icRawDataUpFunc = NULL;

static IcDoorCardOps icCardOps = {
};

//待重写

// 返回值
// >0:数据完整 返回是有效数据长度
	// 如果返回值&0x100 >0 那么说明带有特殊字符，需要在有效数据为+1
//  0:少后半部数据
// -1:数据有错
static int isCompleteness(unsigned char *pRecv, int recvLen)
{
	#define END 0x7e
	#define ESC 0x7d
	int i;
	int escFlag = 0;
	if( pRecv[0] != END)
		return -1;
	if(recvLen > 1)
	{
		if(pRecv[1] != 0xaa)
			return -1;
	}
	for( i = 1; i < recvLen ; i ++)
	{
		if(pRecv[i] == END)
		{
			if(escFlag)
				return (i+1)|0x100;
			else
				return i+1;
		}
		if(pRecv[i] == ESC)
		{
			memcpy(&pRecv[i],&pRecv[i+1],recvLen-i);
			pRecv[i] = changeEsc(pRecv[i]);
			recvLen = recvLen - 1;
			escFlag = 1;
		}
	}
	return 0;
}

static int decodeAlg(const unsigned char *pRecv, int recvLen,
		unsigned char *frameBuf, int * frameBufLen)
{
	int validLen ; //有效数据可以直接处理的数据,已经去掉特殊字符的长度
	int retLen = 0;//已经处理掉的数据
	int doubleValidData = 0;
	getUtilsOps()->printData(pRecv,recvLen);
	validLen = isCompleteness((char *)pRecv,recvLen);

	if(validLen < 0)
	{
		return recvLen; //数据有错，直接做报废处理
	}else if(validLen  == 0)
		return 0; //数据少了后半部分，暂不处理
	else if (validLen &0x100)
	{
		validLen =  validLen & 0xff;
		retLen = validLen + 1;
	}else {
		retLen = validLen;
	}
	memcpy(frameBuf,pRecv+2,validLen-3);
	*frameBufLen = validLen - 3;
	//判断剩下的是否还有整包有效数据
	if(recvLen - retLen > 3 )
	{

		validLen = isCompleteness((char *)pRecv+validLen,recvLen - retLen);
		if(validLen > 0)
		{
			return (retLen|0x10000);
		}
	}
	return retLen;
}

static int parseReader(const unsigned char *pRecv, int recvLen,
		unsigned char *frameBuf, int * frameBufLen) {


#define END 0x7e
#define ESC 0x7d
	getUtilsOps()->printData(pRecv,recvLen);

	int frameLen = 0;
	static int _findHead = 0;
	static int _escflag = 0;
	static int addEscLen =0;
	unsigned char * saveBuf= frameBuf;
	while (recvLen) {
		if (!_findHead) {
			if (*pRecv == END) { //找头部起始字符END
				_findHead = 1;
				frameLen = 0;
				_escflag = 0;
			}
		} else if (*pRecv == END) { //找尾部结束字符END

			if (frameLen >= sizeof(T_READER_HEAD)) {
				int r;
				r = getUtilsOps()->NByteCrc8(0, frameBuf, frameLen);
				if (r == 0) {
					*frameBufLen = frameLen - 1;
					//去掉第一位0XAA
					memcpy(&saveBuf[0],&saveBuf[1],frameLen-1);
					saveBuf[frameLen-1] = 0;
					return frameLen +2+addEscLen; //有效数据+0xaa+0x7e
				} else {
					return 0;
				}
			}
			frameLen = 0;
			_escflag = 0;
		} else if (*pRecv == ESC) {  //遇到特殊字符
			recvLen--;
			if (recvLen == 0) {
				_escflag = 1;
				break;
			}
			pRecv++;
			*frameBuf = changeEsc(*pRecv);
			frameBuf++;
			frameLen++;
			if (frameLen >= READER_MTU)
				_findHead = 0;
		} else {
			if (_escflag) {
				*frameBuf = changeEsc(*pRecv);
				_escflag = 0;
				addEscLen ++;
			} else
				*frameBuf = *pRecv;
			frameBuf++;
			frameLen++;
			if (frameLen >= READER_MTU)
				_findHead = 0;
		}
		pRecv++;
		recvLen--;
	}
	return 0;
}

static unsigned char changeEsc(unsigned char escChar) {
	if ((escChar & 0x20) == 0) {
		return (escChar | 0x20);
	} else {
		return (escChar & 0xdf);
	}
}
static  int uartRecvFunc(unsigned char*data ,unsigned int len)
{
	if(icRawDataUpFunc)
		return icRawDataUpFunc(IC_CARD,data,len);
	return len;
}
pIcDoorCardOps crateFM1702NLOpsServer(const unsigned char *devPath,
		DoorCardRecvFunc  rawUpFunc) {
	if (devPath == NULL)
		goto fail0;
	pIcDoorCardServer IcDoorCardServer = malloc(sizeof(IcDoorCardServer));
	if (IcDoorCardServer == NULL)
		goto fail0;

	IcDoorCardServer->serialOps = createSerialServer(devPath, 57600, 8, 1, 'n');
	if (IcDoorCardServer->serialOps == NULL)
		goto fail1;
	IcDoorCardServer->serialOps->setHandle(IcDoorCardServer->serialOps,uartRecvFunc,decodeAlg,NULL );
	IcDoorCardServer->ops = icCardOps;
	icRawDataUpFunc = rawUpFunc;
	return (pIcDoorCardOps)IcDoorCardServer;

	fail1: free(IcDoorCardServer);
	fail0: return NULL;
}
void destroyFM1702NLOpsServer(pIcDoorCardOps* server) {
	pIcDoorCardServer IcDoorCardServer = (pIcDoorCardOps )*server;
	if(IcDoorCardServer == NULL)
		return ;
	destroySerialServer(&IcDoorCardServer->serialOps);
	free(IcDoorCardServer);

}
