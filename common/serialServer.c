#include<unistd.h>
#include<sys/ioctl.h>
#include<pthread.h>
#include<termios.h>
#include<fcntl.h>
#include<sys/select.h>
#include<errno.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include <sys/epoll.h>
#include <sys/poll.h>
#include <sys/eventfd.h>

#include<sys/select.h>
#include "serialServer.h"
#include "common/threadManage.h"
#include "common/bufferManage.h"
#include "common/debugLog.h"
typedef struct SerialServer {
	SerialOps ops; //提供给外部访问的虚函数表
	int serialDevFd; //打开串口时的设备节点
	pThreadOps recvThreadId; //接收数据的线程
	int stopRecvThreadFd[2]; //用于停止线程的eventfd
	int changeReadModeFd[2];//用于切换同步和异步的读模式
	pThreadOps parseThreadId; //处理数据的线程
	int stopParseThreadFd[2]; //
	pBufferOps bufferOps; //缓存的BUF
	SerialRecvFunc recvFunc; //上报数据给上层的回调函数
	SerialParseFunc parseFunc; //用户层提供的解析数据的算法
	SerialBuildFunc buildFunc; //用户层提供的构造数据的算法
} SerialServer, *pSerialServer;
static int setSerialHandle(struct SerialOps* base, SerialRecvFunc recvFunc,
		SerialParseFunc parseFunc, SerialBuildFunc buildFunc);
static int changeReadMode(struct SerialOps* base,int mode);

//static	int serialRead(struct SerialOps* base,unsigned char * lpBuff,int nBuffSize);
static int serialWrite(struct SerialOps* base, const unsigned char * lpBuff,
		int nBuffSize);
static int serialBaudRate(struct SerialOps* base, int nBaudRate);

static int _setBaudRate(int fd, int nBaudRate);
static int _setDataBits(int fd, int nDataBits);
static int _setStopBits(int fd, int nStopBits);
static int _setParity(int fd, int nParity);
static int _setNoblock(int fd, int bNoBlock);
static int _setOtherOpt(int fd);
static int _read(int fd, unsigned char * lpBuff, int nBuffSize);
static int _readAndWait(struct SerialOps* base, unsigned char * lpBuff,
		int nBuffSize, int nTimeout_ms);
static int _write(int fd, const unsigned char * lpBuff, int nLen);
static int   eventfd_write(int fd, const uint64_t counter);
static int   eventfd_read(int fd,  uint64_t * counter);



static const int BaudRate_LibValue[] = { B115200, B57600, B38400, B19200, B9600,
		B4800, B2400, B1200, B300, B115200, B57600, B38400, B19200, B9600,
		B4800, B2400, B1200, B300, };
static const int BaudRate_Value[] = { 115200, 57600, 38400, 19200, 9600, 4800,
		2400, 1200, 300, 115200, 57600, 38400, 19200, 9600, 4800, 2400, 1200,
		300, };

static SerialOps serialOps = {
		.setHandle = setSerialHandle,
		.setBaudRate = serialBaudRate,
		.read = _readAndWait,
		.write = serialWrite,
		.changeReadMode = changeReadMode,
};


static int   eventfd_write(int fd, const uint64_t counter)
{
	return write(fd, &counter, sizeof(uint64_t));
}
static int   eventfd_read(int fd,  uint64_t * counter)
{
	return read(fd, counter, sizeof(uint64_t));
}


static void * serialRecvThreadFunc(void *arg) {
#define EVENT_NUMS  2
	int epfd, nfds,ret;
	int readLen = 0;
	struct epoll_event ev, events[EVENT_NUMS];
	char readBuff[1024] = { 0 };


	pSerialServer serialServer = *((pSerialServer*) arg);
	if (serialServer == NULL) {
		goto exit;
	}


	epfd = epoll_create(EVENT_NUMS);
	bzero(&ev, sizeof(ev));
	ev.data.fd = serialServer->serialDevFd;
	ev.events = EPOLLIN | POLLPRI;
	epoll_ctl(epfd, EPOLL_CTL_ADD, serialServer->serialDevFd, &ev);
	bzero(&ev, sizeof(ev));
	ev.data.fd = serialServer->stopRecvThreadFd[0];
	ev.events = EPOLLIN | EPOLLPRI;
	epoll_ctl(epfd, EPOLL_CTL_ADD, serialServer->stopRecvThreadFd[0], &ev);

	bzero(&ev, sizeof(ev));
	ev.data.fd = serialServer->changeReadModeFd[0];
	ev.events = EPOLLIN | EPOLLPRI;
	epoll_ctl(epfd, EPOLL_CTL_ADD, serialServer->changeReadModeFd[0], &ev);
	while (serialServer->recvThreadId->check(serialServer->recvThreadId)) {
		nfds = epoll_wait(epfd, events, EVENT_NUMS, -1);
		int i;
		for (i = 0; i < nfds; ++i) {
			if (events[i].data.fd == serialServer->serialDevFd) {
				//*读取数据 并加入缓存队列
				bzero(readBuff, sizeof(readBuff));
				readLen = _read(serialServer->serialDevFd, readBuff,
						sizeof(readBuff));
				//加入队列
				if (readLen > 0)
					serialServer->bufferOps->push(serialServer->bufferOps,
							readBuff, readLen);
				else {
					LOGE("fail to read serialDevFd");
				}
			} else if (events[i].data.fd == serialServer->stopRecvThreadFd[0]) {

				goto exit;
			}else if(events[i].data.fd == serialServer->changeReadModeFd[0]){
				uint64_t counter;
				eventfd_read(serialServer->changeReadModeFd[0],&counter);
				if(counter == SERIAL_READ_ASYNC)
				{
					bzero(&ev, sizeof(ev));
					ev.data.fd = serialServer->serialDevFd;
					ev.events = EPOLLET | EPOLLIN;
					epoll_ctl(epfd, EPOLL_CTL_ADD, serialServer->serialDevFd, &ev);
				}else {
					bzero(&ev, sizeof(ev));
					ev.data.fd = serialServer->serialDevFd;
					epoll_ctl(epfd, EPOLL_CTL_DEL, serialServer->serialDevFd, &ev);
				}
			};
		}
	}
	exit:
	LOGE("serialRecvThreadFunc exit!");
	return NULL;
}

static void * serialParseThreadFunc(void *arg) {
	pSerialServer serialServer = *((pSerialServer*) arg);
	int ret = 0;
	char recvBuf[1024] = { 0 };
	int recvLen;
	int pullLen = 0;
	char validBuf[1024] = { 0 };
	int validLen;
	while (serialServer->recvThreadId->check(serialServer->recvThreadId)) {
		//读取队列数据
		//上报给用户
		ret = serialServer->bufferOps->wait(serialServer->bufferOps);
		switch (ret) {
		case TRI_DATA:
			do {
startPull:
				bzero(recvBuf, sizeof(recvBuf));
				bzero(validBuf, sizeof(validBuf));
				pullLen = serialServer->bufferOps->pull(serialServer->bufferOps,
						recvBuf, sizeof(recvBuf));
				if (pullLen > 0) {
					if (serialServer->recvFunc) {
						if (serialServer->parseFunc) {

							recvLen = serialServer->parseFunc(recvBuf, pullLen,
									validBuf, &validLen);
							if (recvLen > 0) {
								//通过返回值判断是否重复读取
								ret = serialServer->bufferOps->deleteLeft(
										serialServer->bufferOps,
										recvLen & 0xffff);
								if (ret < 0) {
									LOGW("fail to deleteLeft!");
								}
								serialServer->recvFunc(validBuf, validLen);
								if (recvLen & 0x10000){
									LOGE("重复读取\n");
									goto startPull;
								}
							}
						} else {
							recvLen = serialServer->recvFunc(recvBuf, pullLen);
							if (recvLen > 0) {
								//通过返回值判断是否重复读取
								ret = serialServer->bufferOps->deleteLeft(
										serialServer->bufferOps,
										recvLen & 0xffff);
								if (ret < 0) {
									LOGW("fail to deleteLeft!");
								}
								if (recvLen & 0x10000)
									goto startPull;
							}

						}
					}
				}

			} while (pullLen >= sizeof(recvBuf));
			break;
		case TRI_EXIT:
			goto exit;
			break;
		default:
			break;
		}
	}
	exit:
	LOGE("serialRecvThreadFunc exit!");
	return NULL;
}

static int setSerialHandle(struct SerialOps* base, SerialRecvFunc recvFunc,
		SerialParseFunc parseFunc, SerialBuildFunc buildFunc) {
		int ret;
	pSerialServer serialServer = (pSerialServer) base;
	if (serialServer == NULL || recvFunc == NULL)
		goto fail0;
	serialServer->recvThreadId = pthread_register(serialRecvThreadFunc,
			&serialServer, sizeof(pSerialServer), NULL);
	if (serialServer->recvThreadId == NULL)
		goto fail0;

	serialServer->parseThreadId = pthread_register(serialParseThreadFunc,
			&serialServer, sizeof(pSerialServer), NULL);
	if (serialServer->parseThreadId == NULL)
		goto fail1;

	serialServer->bufferOps = createBufferServer(1024);
	if (serialServer->bufferOps == NULL) {
		goto fail2;
	}
	serialServer->recvFunc = recvFunc;
	serialServer->parseFunc = parseFunc;
	serialServer->buildFunc = buildFunc;

	
	ret = pipe(serialServer->stopRecvThreadFd);
	if (ret != 0) {
		goto fail2;
	}
	//设置管道成非阻塞模式
	ret = fcntl(serialServer->stopRecvThreadFd[0], F_SETFL, O_NONBLOCK);
	if (ret != 0) {
		goto fail3;
	}
	ret = fcntl(serialServer->stopRecvThreadFd[1], F_SETFL, O_NONBLOCK);
	if (ret != 0) {
		goto fail3;
	}
	ret = pipe(serialServer->stopParseThreadFd);
	if (ret != 0) {
		goto fail3;
	}
	//设置管道成非阻塞模式
	ret = fcntl(serialServer->stopParseThreadFd[0], F_SETFL, O_NONBLOCK);
	if (ret != 0) {
		goto fail4;
	}
	ret = fcntl(serialServer->stopParseThreadFd[1], F_SETFL, O_NONBLOCK);
	if (ret != 0) {
		goto fail4;
	}
	ret = pipe(serialServer->changeReadModeFd);
	if (ret != 0) {
		goto fail4;
	}
	//设置管道成非阻塞模式
	ret = fcntl(serialServer->changeReadModeFd[0], F_SETFL, O_NONBLOCK);
	if (ret != 0) {
		goto fail5;
	}
	ret = fcntl(serialServer->changeReadModeFd[1], F_SETFL, O_NONBLOCK);
	if (ret != 0) {
		goto fail5;
	}
	

	serialServer->recvThreadId->start(serialServer->recvThreadId);
	serialServer->parseThreadId->start(serialServer->parseThreadId);
	printf("setSerialHandle\n");
	return 0;
			
	fail5:
			close(serialServer->changeReadModeFd[0] );
			close(serialServer->changeReadModeFd[1] );
	fail4:
			close(serialServer->stopParseThreadFd[0] );
			close(serialServer->stopParseThreadFd[1] );
	fail3:
			close(serialServer->stopRecvThreadFd[0] );
			close(serialServer->stopRecvThreadFd[1] );
	fail2: pthread_destroy(&serialServer->parseThreadId);
	fail1: pthread_destroy(&serialServer->recvThreadId);
	fail0: return -1;

}

static int _readAndWait(struct SerialOps* base, unsigned char * lpBuff,
		int nBuffSize, int nTimeout_ms) {
	struct timeval tv;
	fd_set fds;
	int totol, len = 0;
	int ret;
	pSerialServer serialServer = (pSerialServer) base;
	if (serialServer == NULL)
		return -1;
	if (serialServer->serialDevFd < 0) {
		return -1;
	}
	if (lpBuff == NULL || nBuffSize <= 0) {
		return -1;
	}
	FD_ZERO(&fds);
	FD_SET( serialServer->serialDevFd, &fds);
	tv.tv_sec = nTimeout_ms / 1000;
	tv.tv_usec = ((nTimeout_ms) % 1000) * 1000;
	//获取缓冲区数据长度
	do {
		ret = select(serialServer->serialDevFd + 1, &fds, 0, 0, &tv);
	} while (ret == -1 && errno == EINTR);
	switch (ret) {
	case 0:
		LOGW("read timeout!");
		return -2;
		break;
	case -1:
		break;
	default: {
		ioctl(serialServer->serialDevFd, FIONREAD, &totol);
		totol = totol > nBuffSize ? nBuffSize : totol;
		while (len < totol) {
			ret = read(serialServer->serialDevFd, lpBuff + len, totol - len);
			if (ret <= 0) {
				LOGE("fail to read ");
				break;
			}
			len += ret;
		}
	}
		break;
	}
	return len;
}
static int _read(int fd, unsigned char * lpBuff, int nBuffSize) {
	int total, len = 0;
	int ret;
	if (fd < 0) {
		return -1;
	}
	if (lpBuff == 0 || nBuffSize <= 0) {
		return -1;
	}
	ioctl(fd, FIONREAD, &total);
	if (total <= 0)
		return 0;
	total = total > nBuffSize ? nBuffSize : total;
	while (len < total) {

		ret = read(fd, lpBuff + len, total - len);
		if (ret <= 0) {
			LOGE("fail to read ");
			break;
		}
		len += ret;
	}
	return len;
}
static int _write(int fd, const unsigned char * lpBuff, int nLen) {
	int ret;
	int nSendTimes;
	int nSendLen;
	int idx;
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
/*
 static	int serialRead(struct SerialOps* base,unsigned char * lpBuff,int nBuffSize)
 {
 pSerialServer serialServer = (pSerialServer)base;
 if(serialServer == NULL)
 return -1;
 return _read(serialServer->serialDevFd,lpBuff,nBuffSize);
 }
 */
static int serialBaudRate(struct SerialOps* base, int nBaudRate) {
	pSerialServer serialServer = (pSerialServer) base;
	if (serialServer == NULL)
		return -1;
	return _setBaudRate(serialServer->serialDevFd, nBaudRate);
}
static int serialWrite(struct SerialOps* base, const unsigned char * lpBuff,
		int nBuffSize) {

	pSerialServer serialServer = (pSerialServer) base;
	if (serialServer == NULL)
		return -1;
	if (serialServer->buildFunc == NULL) {
		return _write(serialServer->serialDevFd, lpBuff, nBuffSize);
	} else {
		char sendBuf[256] = { 0 };
		int sendLen = 0;
		serialServer->buildFunc(lpBuff, nBuffSize, sendBuf, &sendLen);
		return _write(serialServer->serialDevFd, sendBuf, sendLen);
	}
}
pSerialOps createSerialServer(const char *devPath, int nBaudRate, int nDataBits,
		int nStopBits, int nParity) {
	pSerialServer serialServer = NULL;
	int fd = -1;
	int ret;
	if (devPath == NULL) {
		goto fail0;
	}
	fd = open(devPath, O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd < 0) {
		LOGE("fail to open the %s", devPath);
		goto fail0;
	}
	serialServer = (pSerialServer) malloc(sizeof(SerialServer));
	if (serialServer == NULL) {
		LOGE("fail to malloc serialServer");
		goto fail0;
	}
	bzero(serialServer, sizeof(SerialServer));
	serialServer->serialDevFd = fd;
	serialServer->ops = serialOps;
	/*set Serial parameter */
	ret = _setBaudRate(fd, nBaudRate);
	if (ret < 0) {
		LOGE("fail to setBaudRate!");
		goto fail1;
	}
	ret = _setNoblock(fd, 1);
	if (ret < 0) {
		LOGE("fail to setNoblock!");
		goto fail1;
	}
	ret = _setDataBits(fd, nDataBits);
	if (ret < 0) {
		LOGE("fail to setDataBits!");
		goto fail1;
	}
	ret = _setStopBits(fd, nStopBits);
	if (ret < 0) {
		LOGE("fail to setStopBits!");
		goto fail1;
	}
	ret = _setParity(fd, nParity);
	if (ret < 0) {
		LOGE("fail to setParity!");
		goto fail1;
	}
	ret = _setOtherOpt(fd);
	if (ret < 0) {
		LOGE("fail to setOtherOpt!");
		goto fail1;
	}

	return (pSerialOps) serialServer;
	fail2: free(serialServer);
	fail1: close(fd);
	free(serialServer);
	fail0: return NULL;
}
int changeReadMode(struct SerialOps* base,int mode){

	pSerialServer serialServer = (pSerialServer) base;
	if (serialServer == NULL||serialServer->recvThreadId == NULL)
		return -1;

	return eventfd_write(serialServer->changeReadModeFd[1], mode);
}
void destroySerialServer(pSerialOps * server) {
	pSerialServer serialServer = (pSerialServer) *server;
	if (serialServer == NULL)
		return;
	/*stop recv thread*/
	if (serialServer->recvThreadId != NULL) {
		eventfd_write(serialServer->stopRecvThreadFd[1], 1);
		serialServer->recvThreadId->stop(serialServer->recvThreadId);
		pthread_destroy(&serialServer->recvThreadId);
	}
	if (serialServer->parseThreadId != NULL) {
		serialServer->bufferOps->exitWait(serialServer->bufferOps);
		pthread_destroy(&serialServer->parseThreadId);
	}
	if (serialServer->bufferOps != NULL)
		destroyBufferServer(&serialServer->bufferOps);

	if (serialServer->serialDevFd > 0)
		close(serialServer->serialDevFd);
	if (serialServer->stopRecvThreadFd[1] > 0)
		close(serialServer->stopRecvThreadFd[1]);
	if(serialServer->changeReadModeFd[1] > 0)
		close(serialServer->changeReadModeFd[1]);
	free(serialServer);
	*server = NULL;
}
static int _setBaudRate(int fd, int nBaudRate) {
	int ret;
	int retval = -1;
	int idx;
	struct termios Opt;
	if (fd < 0) {
		return -1;
	}
	tcgetattr(fd, &Opt);
	for (idx = 0; idx < sizeof(BaudRate_LibValue) / sizeof(int); idx++) {
		if (nBaudRate == BaudRate_Value[idx]) {
			tcflush(fd, TCIOFLUSH);
			cfsetispeed(&Opt, BaudRate_LibValue[idx]);
			cfsetospeed(&Opt, BaudRate_LibValue[idx]);
			ret = tcsetattr(fd, TCSANOW, &Opt);
			if (ret != 0) {
				LOGE("tcsetattr");
				break;
			}
			retval = 0;
			break;
		}
		tcflush(fd, TCIOFLUSH);
	}
	return retval;
}
static int _setDataBits(int fd, int nDataBits) {
	int ret;
	struct termios Opt;
	if (fd < 0) {
		return -1;
	}
	ret = tcgetattr(fd, &Opt);
	if (ret != 0) {
		return -1;
	}
	Opt.c_cflag &= ~CSIZE;
	switch (nDataBits) {
	case 5:
		Opt.c_cflag |= CS5;
		break;
	case 6:
		Opt.c_cflag |= CS6;
		break;
	case 7:
		Opt.c_cflag |= CS7;
		break;
	case 8:
		Opt.c_cflag |= CS8;
		break;
	default:
		Opt.c_cflag |= CS8;
		break;
	}
	tcflush(fd, TCIFLUSH);
	ret = tcsetattr(fd, TCSANOW, &Opt);
	if (ret == -1) {
		return -1;
	}
	return 0;
}
static int _setStopBits(int fd, int nStopBits) {
	int ret;
	struct termios Opt;
	if (fd < 0) {
		return -1;
	}
	ret = tcgetattr(fd, &Opt);
	if (ret == -1) {
		return -1;
	}
	switch (nStopBits) {
	case 1:
		Opt.c_cflag &= ~CSTOPB;
		break;
	case 2:
		Opt.c_cflag |= CSTOPB;
		break;
	default:
		Opt.c_cflag &= ~CSTOPB;
		break;
	}
	tcflush(fd, TCIFLUSH);
	ret = tcsetattr(fd, TCSANOW, &Opt);
	if (ret == -1) {
		return -1;
	}
	return 0;
}
static int _setParity(int fd, int nParity) {
	int ret;
	struct termios Opt;
	if (fd < 0) {
		return -1;
	}
	ret = tcgetattr(fd, &Opt);
	if (ret == -1) {
		return -1;
	}
	switch (nParity) {
	case 'n':
	case 'N':
		Opt.c_cflag &= ~PARENB;
		Opt.c_iflag &= ~INPCK;
		break;
	case 'o':
	case 'O':
		Opt.c_cflag |= (PARODD | PARENB);
		Opt.c_iflag |= INPCK;
		break;
	case 'e':
	case 'E':
		Opt.c_cflag |= PARENB;
		Opt.c_cflag &= ~PARODD;
		Opt.c_iflag |= INPCK;
		break;
	default:
		Opt.c_cflag &= ~PARENB;
		Opt.c_iflag &= ~INPCK;
		break;
	}
	if (nParity != 'n')
		Opt.c_iflag |= INPCK;
	tcflush(fd, TCIFLUSH);
	ret = tcsetattr(fd, TCSANOW, &Opt);
	if (ret == -1) {
		return -1;
	}
	return 0;
}
static int _setNoblock(int fd, int bNoBlock) {
	if (fd < 0)
		return -1;
	if (ioctl(fd, FIONBIO, &bNoBlock) < 0)
		return -1;
	return 0;
}

static int _setOtherOpt(int fd) {
	int ret;
	struct termios Opt;
	if (fd < 0) {
		return -1;
	}
	ret = tcgetattr(fd, &Opt);
	if (ret == -1) {
		return -1;
	}
	Opt.c_iflag &= ~(IGNBRK | IGNCR | INLCR | ICRNL | IUCLC | IXANY | IXON
			| IXOFF | INPCK | ISTRIP);
	Opt.c_iflag |= (BRKINT | IGNPAR);
	Opt.c_oflag &= ~OPOST;
	Opt.c_lflag &= ~(XCASE | ECHONL | NOFLSH);
	Opt.c_lflag &= ~(ICANON | ISIG | ECHO);
	Opt.c_cflag |= (CLOCAL | CREAD);
	Opt.c_cc[VTIME] = 1;
	Opt.c_cc[VMIN] = 128;
	tcflush(fd, TCIFLUSH);
	ret = tcsetattr(fd, TCSANOW, &Opt);
	if (ret == -1) {
		return -1;
	}
	return 0;
}
