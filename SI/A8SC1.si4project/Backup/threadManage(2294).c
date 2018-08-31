#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "taskManage/threadManage.h"
#include "common/debugLog.h"


typedef struct PthreadClass{
	ThreadOps ops;
	pthread_t  thread;
	ThreadFunc func;
    pthread_mutex_t _mutex;
	ThreadRunState workState;
	pthread_attr_t  *attr;
	void * arg;
	int   len;
}PthreadClass,*pPthreadClass;
static int pthread_start(pThreadOps base);
static int pthread_stop(pThreadOps base);
ThreadRunState pthread_checkRunState(pThreadOps base);

ThreadOps threadOps = {
		.start=pthread_start,
		.check=pthread_checkRunState,
		.stop=pthread_stop,
};
pThreadOps pthread_register(ThreadFunc callback,void *arg,int datalen,pthread_attr_t  *attr)
{
	pPthreadClass pClass =  malloc(sizeof(PthreadClass));
	if(pClass == NULL)
	{
		LOGE("fail to malloc pPthreadClass");
		goto fail0;
	}
	bzero((void*)pClass,sizeof(PthreadClass));
	if(arg &&( datalen>0) )
	{
		pClass->arg = malloc(datalen);
		if(pClass->arg == NULL)
		{
				LOGE("fail to malloc pClass->arg ");
				goto fail1;
		}
		memcpy(pClass->arg,arg ,datalen);
	}
	if(attr){
		pClass->attr = malloc(sizeof(pthread_attr_t));
		if(pClass->arg == NULL)
		{
				LOGE("fail to malloc pClass->attr ! ");
				goto fail2;
		}
		memcpy(pClass->attr, attr,sizeof(pthread_attr_t));
	}
	if(callback )
	{
		pClass->func = callback;
	}else
		goto fail3;
	pthread_mutex_init(&pClass->_mutex,NULL);
	pClass->workState = Thread_Stop;
	pClass->ops = threadOps;
	return (pThreadOps)pClass;
	fail3:
		free(pClass->attr);
	fail2:
		free(pClass->arg);
	fail1:
		free(pClass);
	fail0:
		return NULL;
}

ThreadRunState pthread_checkRunState(pThreadOps base)
{
	ThreadRunState state;
	if(base == NULL )
			return -1;
	
	pPthreadClass this = (pPthreadClass)(base);
	pthread_mutex_lock(&this->_mutex);
	state = this->workState;
	pthread_mutex_unlock(&this->_mutex);
	return state;
	
}
static int pthread_start(pThreadOps base)
{	
	pPthreadClass pthreadclass = (pPthreadClass)base;
	if(pthreadclass == NULL)
			return -1;
	pthread_mutex_lock(&pthreadclass->_mutex);
	pthreadclass->workState = Thread_Run;
	pthread_mutex_unlock(&pthreadclass->_mutex);
	if (pthread_create(&(pthreadclass->thread),pthreadclass->attr,pthreadclass->func,
			pthreadclass->arg) != 0) {
		return -1;
	}
	return 0;
}
int pthread_destroy(pThreadOps * pthread)
{
	if(pthread == NULL ||*pthread == NULL)
		return -1;
	pPthreadClass this = (pPthreadClass)(*pthread);
	pthread_stop(this);
	if(this->arg){
		free(this->arg);
		this->arg = NULL;
	}
	if(this->attr)
	{
		free(this->attr);
		this->attr = NULL;
	}
	pthread_mutex_destroy(&this->_mutex);
	free(this);
	*pthread = NULL;
	return 0;
	fail0:
		return -1;
}
static int pthread_stop(pThreadOps base)
{
	void * tret;
	int err;
	if(base == NULL )
		return -1;
	pPthreadClass this = (pPthreadClass)(base);
	pthread_mutex_lock(&this->_mutex);
	if(this->workState == Thread_Stop ){
		pthread_mutex_unlock(&this->_mutex);
		LOGD("pthread have already stopped!");
		return 0;
	}
	
	this->workState = Thread_Stop ;
	pthread_mutex_unlock(&this->_mutex);
	err = pthread_join(this->thread,&tret);
	if(err != 0){
	        LOGE("can't join thread : %s\n",strerror(err));
			return -1;
	}
	return 0;
}









