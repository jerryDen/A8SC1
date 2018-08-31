#ifndef  __HTTP_CLIENT__H__
#define  __HTTP_CLIENT__H__

#include <curl/curl.h>
/*
供外部调用的函数操作集
setopt 设置参数 
参数1：需要设置参数的枚举
参数2: 设置值

read 读取数据
参数1:数据buf指针
参数2:数据buf长度
参数3:等待超时时间(ms)

*/

typedef struct HttpClientOps{

		void (*setopt)(struct HttpClientOps *,int,const char *);
		int (*read)(struct HttpClientOps* ,void *,int,int);
		int (*readurl)(struct HttpClientOps*,const char *  ,void * ,int  ,int );
}HttpClientOps,*pHttpClientOps;


pHttpClientOps createHttpClientServer(void);
void   destroyHttpClientServer(pHttpClientOps *ops);









#endif





































