#ifndef __THREAD_MANAGE_H_
#define __THREAD_MANAGE_H_

#include <string.h>
#include <unistd.h>
#include <pthread.h>

typedef enum ThreadRunState{
	Thread_Stop = 0,
	Thread_Run,
}ThreadRunState;
typedef void *(*ThreadFunc)(void*);
/*
	threadfunc:
		init add:
			pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
			pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);
		loop add:
			pthread_testcancel();
*/
typedef struct ThreadOps{
	int (*start)(struct ThreadOps *);
	ThreadRunState (*check)(struct ThreadOps *);
	int (*stop) (struct ThreadOps *);
}ThreadOps,*pThreadOps;
pThreadOps
pthread_register(ThreadFunc callback,void *arg,
	int datalen,pthread_attr_t  *attr);
int 
pthread_destroy(pThreadOps * pthread);
#endif










