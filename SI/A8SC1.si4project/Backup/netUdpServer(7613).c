#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/select.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/poll.h>
#include <sys/eventfd.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdint.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <setjmp.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "common/threadManage.h"
#include "common/bufferManage.h"
#include "common/debugLog.h"
#include "common/Utils.h"
#include "common/netUdpServer.h"
typedef struct SerialServer{
	UdpOps ops; //提供给外部访问的虚函数表
	int socketFd; //打开串口时的设备节点
	pThreadOps recvThreadId; //接收数据的线程
	int stopRecvThreadFd; //用于停止线程的eventfd
	pThreadOps parseThreadId;   //处理数据的线程
	int stopParseThreadFd; 		//
	struct sockaddr_in remoteInfo;//发送端的网络信息
	pBufferOps   bufferOps; //缓存的BUF
	UDPRecvFunc  recvFunc; //上报数据给上层的回调函数
	UDPParseFunc parseFunc; //用户层提供的解析数据的算法
	UDPBuildFunc buildFunc; //用户层提供的构造数据的算法
}UdpServer,*pUdpServer;
static int  setHandle(struct UdpOps* base,UDPRecvFunc recvFunc,
	UDPParseFunc  parseFunc ,UDPBuildFunc  buildFunc);
static int  udp_getSocketFd(int bandPort);
static int  _readAndWait(struct UdpOps* base,unsigned char * lpBuff,int nBuffSize,int nTimeout_ms);
static int  _udpWrite( struct UdpOps* base, const char * data, int size,
		uint32_t ipAddr, int port);
static	int _read(int fd,unsigned char * lpBuff,int nBuffSize);
static	int _write(int fd,const unsigned char * lpBuff,int nLen);
static int  socketBind(const int sockfd, const int port);
static int  _setNoblock(int fd, int bNoBlock);
static int  _ack (struct UdpOps* base,unsigned char * data ,int size);
static int joinMulticast(struct UdpOps* base,unsigned int multiaddr);
static int getRemoteInfo(struct UdpOps* base,struct sockaddr_in *remoteInfo);
static int _setsockopt(struct UdpOps* base, int level, int optname,const void *optval, socklen_t optlen);
static int setRemoteInfo(struct UdpOps* base,struct sockaddr_in remoteInfo);

static int   eventfd_write(int fd, const uint64_t counter);


static UdpOps ops = {
		.setHandle = setHandle,
		.read  =  _readAndWait,
		.ack   =  _ack,
		.write =  _udpWrite,
		.joinMulticast = joinMulticast,
		.getRemoteInfo = getRemoteInfo,
		.setRemoteInfo = setRemoteInfo,
		.setsockopt = _setsockopt,
};


static int   eventfd_write(int fd, const uint64_t counter)
{
	return write(fd, &counter, sizeof(uint64_t));
}

static  void * udpRecvThreadFunc(void *arg)
{
	#define EVENT_NUMS  2
	int epfd, nfds;
	int readLen = 0;
	struct epoll_event ev, events[EVENT_NUMS];
	char readBuff[1024] = {0};
	pUdpServer udpServer  =*((pUdpServer*)arg);
	LOGD("udpRecvThreadFunc");
	if(udpServer == NULL)
	{
		goto exit;
	}
	udpServer->stopRecvThreadFd =  eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	if(udpServer->stopRecvThreadFd < 0 )
	{
		goto exit;
	}
	epfd = epoll_create(EVENT_NUMS);
	bzero(&ev,sizeof(ev));
	ev.data.fd = udpServer->socketFd;
	ev.events = EPOLLET|EPOLLIN;
	epoll_ctl(epfd, EPOLL_CTL_ADD, udpServer->socketFd, &ev);

	bzero(&ev,sizeof(ev));
	ev.data.fd = udpServer->stopRecvThreadFd;
	ev.events = EPOLLIN|EPOLLPRI;
	epoll_ctl(epfd, EPOLL_CTL_ADD, udpServer->stopRecvThreadFd, &ev);

	while(udpServer->recvThreadId->check(udpServer->recvThreadId))
	{
		nfds = epoll_wait(epfd, events, EVENT_NUMS, -1);
		
		int i;
		for (i = 0; i < nfds; ++i) {
			if (events[i].data.fd == udpServer->socketFd)
			{
				//*读取数据 并加入缓存队列
				int sockaddrLen = sizeof(struct sockaddr);
				bzero(readBuff,sizeof(readBuff));
			//	readLen = _read(udpServer->socketFd,readBuff,sizeof(readBuff));
				readLen =  recvfrom(udpServer->socketFd,readBuff, sizeof(readBuff), 0,
						(struct sockaddr*)&udpServer->remoteInfo, (socklen_t*)&sockaddrLen);
				//加入队列
				if( readLen > 0 )
				{	
					udpServer->bufferOps->push(udpServer->bufferOps,readBuff,readLen);
				}
				else{
					LOGE("fail to _read serialDevFd");
				}
			}else if(events[i].data.fd  == udpServer->stopRecvThreadFd)
			{
				goto exit;
			}
		}
	}
exit:
	LOGE("udpRecvThreadFunc exit!");
	return NULL;
}

static  void * UdpParseThreadFunc(void *arg)
{
	pUdpServer udpServer  =*((pUdpServer*)arg);
	if(udpServer == NULL)
	{
		goto exit;
	}
	int ret = 0;
	char recvBuf[2046] = {0};
	char validBuf[2046] = {0};
	int  handleLen = 0;
	int  pullLen = 0;
	int  validLen;
	LOGD("UdpParseThreadFunc");
	while(udpServer->recvThreadId->check(udpServer->recvThreadId))
	{
		//读取队列数据
		//上报给用户
		ret = udpServer->bufferOps->wait(udpServer->bufferOps);
		switch(ret){
			case TRI_DATA:
					do{
						bzero(recvBuf,sizeof(recvBuf));
						bzero(validBuf,sizeof(validBuf));
						pullLen = udpServer->bufferOps->pull(udpServer->bufferOps,recvBuf,sizeof(recvBuf));
						if(pullLen > 0)
						{
							if(udpServer->recvFunc)
							{
								if(udpServer->parseFunc ){
									handleLen = udpServer->parseFunc(recvBuf,pullLen,validBuf,&validLen);
									if((handleLen >0) )
									{
										if( validLen>0 ){
											ret = udpServer->bufferOps->deleteLeft(udpServer->bufferOps,handleLen);
											if(ret < 0)
											{
												LOGW("fail to deleteLeft!");
											}
											udpServer->recvFunc(validBuf,validLen);
										}else{
											LOGW("No valid data was found");
											break;
										}
									}else {
										break;
									}
								}else {
									handleLen = udpServer->recvFunc(recvBuf,pullLen);
									if(handleLen >0)
									{
										ret = udpServer->bufferOps->deleteLeft(udpServer->bufferOps,handleLen);
										if(ret < 0)
										{
											LOGW("fail to deleteLeft!");
										}
									}else{
										break;
									}
								}
								pullLen -= handleLen;
							}
							}else{
								break;
							}
					}while(pullLen >= handleLen);
				break;
			case TRI_EXIT:
					goto exit;
				break;
			default:
				break;
		}
	}
 exit:
	LOGE("UdpParseThreadFunc exit!");
	return NULL;
}


static int setHandle(struct UdpOps* base,UDPRecvFunc recvFunc,
	UDPParseFunc  parseFunc ,UDPBuildFunc  buildFunc)
{
	pUdpServer udpServer = (pUdpServer)base;
	if(udpServer == NULL)
				goto fail0;
	udpServer->parseThreadId = pthread_register(UdpParseThreadFunc,&udpServer,
			sizeof(pUdpServer),NULL);
	if(udpServer->parseThreadId == NULL)
	{
		goto fail0;
	}
	udpServer->recvThreadId = pthread_register(udpRecvThreadFunc,&udpServer,
				sizeof(pUdpServer),NULL);
	if(udpServer->recvThreadId == NULL)
	{
		goto fail1;
	}
	udpServer->bufferOps = createBufferServer(1024);
	if(udpServer->bufferOps ==NULL)
	{
		goto fail2;
	}
	udpServer->recvFunc = recvFunc;
	udpServer->parseFunc = parseFunc;
	udpServer->buildFunc = buildFunc;

	udpServer->recvThreadId->start(udpServer->recvThreadId);
	udpServer->parseThreadId->start(udpServer->parseThreadId);

	return true;
fail2:
	free(udpServer->recvThreadId);
fail1:
	free(udpServer->parseThreadId);
fail0:
	return -1;
}
static int  _ack (struct UdpOps* base,unsigned char * data ,int size)
{
	pUdpServer udpServer = (pUdpServer)base;
	if(udpServer == NULL)
				goto fail0;
	
	return _udpWrite(base,data,size,(udpServer->remoteInfo.sin_addr.s_addr)
			,ntohs(udpServer->remoteInfo.sin_port));
fail0:
	return -1;
}

static	int _readAndWait(struct UdpOps* base,unsigned char * lpBuff,int nBuffSize,int nTimeout_ms)
{
	struct timeval		tv;
	fd_set				fds;
	int totol,len = 0;
	int ret;
	pUdpServer	udpServer  = (pUdpServer)base;
		if(udpServer == NULL)
				return -1;
	if (udpServer->socketFd < 0) {
		return -1;
	}
	if (lpBuff == NULL || nBuffSize <= 0) {
		return -1;
	}
	FD_ZERO(&fds);
	FD_SET(	udpServer->socketFd, &fds);
	tv.tv_sec = nTimeout_ms/ 1000;
	tv.tv_usec = ((nTimeout_ms)% 1000) * 1000;
	//获取缓冲区数据长度
	do {
		ret = select(udpServer->socketFd + 1, &fds, 0, 0, &tv);
	} while (ret == -1 && errno == EINTR);
	switch(ret){
		case 0:
				LOGW("read timeout!");
				return -2;
			break;
		case -1:
				break;
		default:{
				ioctl(	udpServer->socketFd, FIONREAD, &totol);
				totol = totol > nBuffSize ? nBuffSize : totol;
				while(len < totol){
					ret = read(udpServer->socketFd, lpBuff + len, totol - len);
					if (ret <= 0) {
							LOGE("fail to read ");
							break;
					}
					len += ret;
				}
			}break;
		}
	return len;
}
static	int _read(int fd,unsigned char * lpBuff,int nBuffSize)
{
	int totol,len = 0;
	int ret;
	if (fd < 0) {
		return -1;
	}
	if (lpBuff == 0 || nBuffSize <= 0) {
		return -1;
	}
	ioctl(fd, FIONREAD, &totol);
	if(totol <=0)
		return 0;
	totol = totol > nBuffSize ? nBuffSize : totol;
	while (len < totol) {

		ret = read(fd, lpBuff + len, totol - len);
		if (ret <= 0) {
			LOGE("fail to read ");
			break;
		}
		len += ret;
	}
	return len;
}
static	int _write(int fd,const unsigned char * lpBuff,int nLen)
{
	int				ret;
	int				nSendTimes;
	int				nSendLen;
	int				idx;
	if (fd < 0 || lpBuff == 0 || nLen <= 0) {
		return -1;
	}
	nSendTimes = 0;
	nSendLen = 0;
	while (nSendTimes++ < 5 && nSendLen < nLen) {
		ret = write(fd, lpBuff + nSendLen, nLen - nSendLen);
		if (ret <= 0) {
			break;
		}
		nSendLen += ret;
	}
	return nSendLen;
}


pUdpOps createUdpServer(int netPort)
{
	pUdpServer udpServer = NULL;
	int	fd = -1;
	int	ret;
	udpServer = (pUdpServer)malloc(sizeof(UdpServer));
	if(udpServer == NULL)
		goto fail0;
	bzero(udpServer,sizeof(UdpServer));
	udpServer->socketFd = udp_getSocketFd(netPort);
	if(udpServer->socketFd < 0)
		goto fail1;

	_setNoblock(udpServer->socketFd ,1);
	udpServer->ops = ops;
	return (pUdpOps)udpServer;
	fail1:
		free(udpServer);
	fail0:
		return NULL;
}
static int _setsockopt(struct UdpOps* base, int level, int optname,const void *optval, socklen_t optlen)
{
	pUdpServer	udpServer  = (pUdpServer)base;
		if(udpServer == NULL)
				return -1;
	return setsockopt(udpServer->socketFd,level,optname,optval,optlen);
}
static int _udpWrite(struct UdpOps* base, const char * data, int size,
		uint32_t ipAddr, int port) {
	int ret,i;
	struct sockaddr_in remote_addr;
	pUdpServer	udpServer  = (pUdpServer)base;
			if(udpServer == NULL)
					return -1;
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port = htons(port); //htons
	remote_addr.sin_addr.s_addr = ipAddr;
	bzero(&(remote_addr.sin_zero),sizeof(remote_addr.sin_zero));	

	ret = sendto(udpServer->socketFd, data, size, 0, (struct sockaddr*) &remote_addr,
				(socklen_t) sizeof(struct sockaddr));
	if (ret == -1) {
		LOGE("writeDatagram  failed:%s\n", strerror(errno));
	}
	return ret;
}
static int udp_getSocketFd(int bandPort)
{
	int udpSocketfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (udpSocketfd <= 0) {
		LOGE("fail to socket ");
		return -1;
	}
	int broadcastPerm = 1;

    if(setsockopt(udpSocketfd,SOL_SOCKET,SO_BROADCAST,&broadcastPerm,sizeof(broadcastPerm))<0)
    {
        LOGE("setsockopt() failed!");
    }
	if(socketBind( udpSocketfd,bandPort) == 0)
		return udpSocketfd;
	else{
		close(udpSocketfd);
		return -1;
	}

}
static int setRemoteInfo(struct UdpOps* base,struct sockaddr_in remoteInfo)
{
	pUdpServer	udpServer  = (pUdpServer)base;
		if(udpServer == NULL)
			return -1;
	udpServer->remoteInfo = remoteInfo;
	return 0;
	
}

static int getRemoteInfo(struct UdpOps* base,struct sockaddr_in *remoteInfo)
{
		pUdpServer	udpServer  = (pUdpServer)base;
		if(udpServer == NULL||remoteInfo == NULL)
			return -1;
		if(udpServer->remoteInfo.sin_addr.s_addr == 0)
			return -1;
		*remoteInfo  = udpServer->remoteInfo;
	
		return 0;
}
static int joinMulticast(struct UdpOps* base,unsigned int multiaddr)
{
		pUdpServer	udpServer  = (pUdpServer)base;
			if(udpServer == NULL)
				return -1;
		struct ip_mreq mreq;								
		mreq.imr_multiaddr.s_addr = multiaddr;				
		mreq.imr_interface.s_addr = htonl(INADDR_ANY);		
		if( setsockopt( udpServer->socketFd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq) ) < 0){
			LOGE("IP_ADD_MEMBERSHIP\n");
			//return -1;
		}
		int on = 1;
		//设置允许发生广播
		if( setsockopt(udpServer->socketFd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0 ){
			LOGE("as_socket setsockopt SO_BROADCAST:");
			return	-1;
		}
		if(setsockopt(udpServer->socketFd,IPPROTO_IP,IP_MULTICAST_IF,&mreq.imr_multiaddr,sizeof(mreq.imr_multiaddr)) < 0)
		{
			LOGE("IP_MULTICAST_IF\n");
			//return -1;
		}
		int loop = 0;
		if(setsockopt(udpServer->socketFd,IPPROTO_IP,IP_MULTICAST_LOOP,&loop,sizeof(loop)) < 0)
		{
			LOGE("IP_MULTICAST_LOOP\n");
			//return -1;
		}
		return 0;
}
static int socketBind(const int sockfd, const int port) {
	struct sockaddr_in m_addr;
	m_addr.sin_family = AF_INET;
	m_addr.sin_addr.s_addr = htons(INADDR_ANY);
	m_addr.sin_port = htons(port);//�����ֽ���
	bzero(&(m_addr.sin_zero), 8);

	int opt = SO_REUSEADDR;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	int bind_return = bind(sockfd, (struct sockaddr*) &m_addr, sizeof(m_addr));

	if (bind_return == -1) {
		LOGE("bind failed: %s\n", strerror(errno));
		return false;
	}
	return true;
}
/*
static int   eventfd_write(int fd, const uint64_t counter)
{
	return write(fd, &counter, sizeof(uint64_t));
}*/

void destroyUdpServer(pUdpOps * server)
{
	pUdpServer udpServer = (pUdpServer)*server;
	if(udpServer == NULL)
				return ;
	/*stop recv thread*/
	if(udpServer->recvThreadId != NULL){
		eventfd_write(udpServer->stopRecvThreadFd,1);
		udpServer->recvThreadId->stop(udpServer->recvThreadId);
		pthread_destroy(&udpServer->recvThreadId);
	}
	if(udpServer->parseThreadId != NULL)
	{
		udpServer->bufferOps->exitWait(udpServer->bufferOps);
		pthread_destroy(&udpServer->parseThreadId);
	}
	if(udpServer->bufferOps != NULL )
		destroyBufferServer(&udpServer->bufferOps);

	if(udpServer->socketFd > 0 )
		close(udpServer->socketFd);
	if( udpServer->stopRecvThreadFd > 0)
		close(udpServer->stopRecvThreadFd);
}
static int _setNoblock(int fd, int bNoBlock)
{
	if (fd < 0)
		return -1;
	if (ioctl(fd, FIONBIO, &bNoBlock) < 0)
		return -1;
	return 0;
}

