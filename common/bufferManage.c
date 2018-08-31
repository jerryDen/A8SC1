#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/poll.h>
#include <sys/eventfd.h>
#include <pthread.h>
#include "common/bufferManage.h"
#include "common/debugLog.h"
#define RESERVED (512)
#define AUGMENT_SIZE 512

#define MAX_SIZE (2*1024)

typedef struct BufferServer{
	BufferOps ops;
	void *buffer;
	int  bufferTotalLen;
	int  validLen;
	int  epoolFd;
	int  isExit;
	int wakeFds[2]; //[0]:read [1]:write
	pthread_mutex_t   _mutex;
}BufferServer,*pBufferServer;

static int exitWait(struct BufferOps* base);
static int pullWait(struct BufferOps* base);
static int deleteLeft(struct BufferOps* base ,int offset);
static int pull(struct BufferOps* base,void * data,int len);
static int push(struct BufferOps* base,void * data,int len);


static BufferOps bufferOps = {
		.push = push,
		.pull = pull,
		.deleteLeft = deleteLeft,
		.wait = pullWait,
		.exitWait = exitWait,
};
static int push(struct BufferOps* base,void * data,int len)
{
	pBufferServer pthis = (pBufferServer)base;
	int nWrite, pushCmd = TRI_DATA;
	if(pthis == NULL )
	{
		goto fail0;
	}
	pthread_mutex_lock(&pthis->_mutex);
	if(pthis->validLen + len  > pthis->bufferTotalLen){
		if(pthis->bufferTotalLen+RESERVED < MAX_SIZE ){

			pthis->buffer = realloc(pthis->buffer ,pthis->bufferTotalLen+RESERVED);
			pthis->bufferTotalLen += RESERVED;

		}else
		{
			pthread_mutex_unlock(&pthis->_mutex);
			LOGE("buffer size is too long!");
			goto fail0;
		}
	}
	memcpy(pthis->buffer+pthis->validLen,data,len);
	pthis->validLen += len;
	pthread_mutex_unlock(&pthis->_mutex);
	do {
	     nWrite = write(pthis->wakeFds[1], &pushCmd, sizeof(pushCmd));
	} while (nWrite == -1 );
	return nWrite;
fail1:
	free(pthis);
fail0:
	return -1;
}
static int pullWait(struct BufferOps* base)
{
	struct epoll_event events[1];
	int recvCmd ;
	eventfd_t counter = -1;
	int ret;
	pBufferServer pthis = (pBufferServer)base;
	if(pthis == NULL )
	{
			goto fail0;
	}
	epoll_wait(pthis->epoolFd, events, 1, -1);

	ret = read(pthis->wakeFds[0],&recvCmd,sizeof(recvCmd));
	if(recvCmd == TRI_EXIT){
		pthread_mutex_lock(&pthis->_mutex);
		pthis->isExit = 1;
		pthread_mutex_unlock(&pthis->_mutex);
	}
	return recvCmd;
fail0:
	return -1;
}
static int pull(struct BufferOps* base,void * data,int len)
{
	if(data == NULL||len<=0)
		return -1;
	pBufferServer pthis = (pBufferServer)base;
	if(pthis == NULL )
	{
		goto fail0;
	}
	pthread_mutex_lock(&pthis->_mutex);
	int copyLen = pthis->validLen>len?len:pthis->validLen;
	memcpy(data,pthis->buffer,copyLen);
	pthread_mutex_unlock(&pthis->_mutex);
	return copyLen;
	fail0:
		return -1;
}
static int deleteLeft(struct BufferOps* base ,int offset)
{
	if( offset <=0)
		return -1;
	pBufferServer pthis = (pBufferServer)base;
	if(pthis == NULL )
	{
		goto fail0;
	}
	pthread_mutex_lock(&pthis->_mutex);
	offset = offset > pthis->validLen?pthis->validLen:offset;
	memset(pthis->buffer,0,offset);
	memcpy(pthis->buffer,pthis->buffer+offset,pthis->validLen-offset );
	if(pthis->bufferTotalLen - pthis->validLen > RESERVED)
	{
		pthis->buffer = realloc(pthis->buffer ,pthis->bufferTotalLen-RESERVED);
		pthis->bufferTotalLen -= RESERVED;
	}
	pthis->validLen -= offset;
	pthread_mutex_unlock(&pthis->_mutex);

	return offset;
	fail0:
	return -1;
}

static int exitWait(struct BufferOps* base)
{
	pBufferServer pthis = (pBufferServer)base;
	int nWrite;
	int exitCmd = TRI_EXIT;
	if(pthis == NULL )
	{
		goto fail0;
	}
	do {
	     nWrite = write(pthis->wakeFds[1],&exitCmd, sizeof(exitCmd));
	} while (nWrite == -1 );
	return nWrite;
fail0:
	return -1;
}

pBufferOps createBufferServer(int size)
{
	pBufferServer bufferServer = malloc(sizeof(BufferServer));
	int result;
	if(bufferServer == NULL)
	{
		goto fail0;
	}
	bzero(bufferServer,sizeof(BufferServer ));
	bufferServer->buffer = malloc(size);
	if(bufferServer == NULL)
	{
		goto fail1;
	}
	bzero(bufferServer->buffer,size);
	bufferServer->bufferTotalLen = size;
	bufferServer->validLen = 0;
	bufferServer->ops = bufferOps;

	struct epoll_event ev, events[1];
	bufferServer->epoolFd = epoll_create(1);
	if(bufferServer->epoolFd <0 )
	{
		goto fail2;
	}

	result = pipe(bufferServer->wakeFds);
	if(result != 0)
	{
		goto fail2;
	}
	result = fcntl(bufferServer->wakeFds[0], F_SETFL, O_NONBLOCK);
	if(result != 0)
	{
		goto fail2;
	}
	result = fcntl(bufferServer->wakeFds[1], F_SETFL, O_NONBLOCK);
	if(result != 0)
	{
		goto fail2;
	}
	bzero(&ev,sizeof(ev));
	ev.data.fd = bufferServer->wakeFds[0];
	ev.events = EPOLLIN ;
	epoll_ctl(bufferServer->epoolFd, EPOLL_CTL_ADD,bufferServer->wakeFds[0], &ev);
	pthread_mutex_init(&bufferServer->_mutex,NULL);
	return (pBufferOps)bufferServer;
	fail2:
		free(bufferServer->buffer);
	fail1:
		free(bufferServer);
	fail0:
		return NULL;
}
void destroyBufferServer(pBufferOps *base)
{
	int timeOut = 0;
	pBufferServer pthis = (pBufferServer)( *base);
	if(pthis == NULL )
	{
		return;
	}
	do{
		exitWait(*base);
		LOGE("WAIT EXIT!!!!!!!!!!!!");
		pthread_mutex_lock(&pthis->_mutex);
		if(pthis->isExit == 1){
			usleep(100*1000);
			pthread_mutex_unlock(&pthis->_mutex);
			break;
		}
		pthread_mutex_unlock(&pthis->_mutex);
		
		usleep(10*1000);
		if(timeOut++ >100){
			LOGE("fail to destroyBufferServer timeout!");
			break;
		}
	}while(1);
	close(pthis->wakeFds[0]);
	close(pthis->wakeFds[1]);
	close(pthis->epoolFd);
	pthread_mutex_destroy(&pthis->_mutex);
	if(pthis->buffer != NULL)
		free(pthis->buffer);
	pthis->buffer = NULL;
	free(pthis);
	*base = NULL;
}



