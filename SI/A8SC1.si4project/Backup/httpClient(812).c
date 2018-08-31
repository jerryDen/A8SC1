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

#include "httpClient.h"
#include "curl/curl.h"
#include "curl/multi.h"

typedef struct HttpClientServer{

	HttpClientOps  ops;
	CURL *http_handle;
	CURLM *multi_handle;
	int still_running;
    char *buffer;               /* buffer to store cached data*/
    int buffer_len;             /* currently allocated buffers length */
    int buffer_pos;
}HttpClientServer,*pHttpClientServer;

static void url_setopt(pHttpClientOps ops,int cmd,const char * parameters);
static int  url_read(pHttpClientOps ops,void * data,int len ,int timeout);
static int  url_readurl(pHttpClientOps ops,const char * url ,void * data,int len ,int timeout);
static int  url_post(pHttpClientServer ops,const char * url ,void * post,char *retdata ,int retdataLen);

static int download(char * url,char * outfile)  ;

static int url_port_read(pHttpClientOps ops,void * data,int len ,int timeout);

static size_t 
write_callback(char *buffer,
               size_t size,
               size_t nitems,
               void *userp);






static HttpClientOps  ops = {
			.setopt = url_setopt,
			.read = url_read,
			.readurl = url_readurl,
			.download = download,
			.urlpost = url_post,
};
static int
use_buffer(pHttpClientServer server,int want)
{
    /* sort out buffer */
    if((server->buffer_pos - want) <=0)
    {
        /* ditch buffer - write will recreate */
        if(server->buffer)
            free(server->buffer);

        server->buffer=NULL;
        server->buffer_pos=0;
        server->buffer_len=0;
    }
    else
    {
        /* move rest down make it available for later */
        memmove(server->buffer,
                &server->buffer[want],
                (server->buffer_pos - want));
        server->buffer_pos -= want;
    }
    return 0;
}


static int
fill_buffer(pHttpClientServer server,int want,int waittime)
{
    fd_set fdread;
    fd_set fdwrite;
    fd_set fdexcep;
    int maxfd;
    struct timeval timeout;
    int rc;

    /* only attempt to fill buffer if transactions still running and buffer
     * doesnt exceed required size already
     */
    if((!server->still_running) || (server->buffer_pos > want))
        return 0;
    /* attempt to fill buffer */
    do
    {
        FD_ZERO(&fdread);
        FD_ZERO(&fdwrite);
        FD_ZERO(&fdexcep);

        /* set a suitable timeout to fail on */
        timeout.tv_sec = waittime; /* 1 minute */
        timeout.tv_usec = 0;
        /* get file descriptors from the transfers */
        curl_multi_fdset(server->multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);
        /* In a real-world program you OF COURSE check the return code of the
           function calls, *and* you make sure that maxfd is bigger than -1
           so that the call to select() below makes sense! */
        rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);
        switch(rc) {
        case -1:
            /* select error */
            break;
        case 0:
			 fprintf(stderr,"select timeout\n");
			// return 0;
        
        default:
            /* timeout or readable/writable sockets */
            /* note we *could* be more efficient and not wait for
             * CURLM_CALL_MULTI_PERFORM to clear here and check it on re-entry
             * but that gets messy */
            while(curl_multi_perform(server->multi_handle, &server->still_running) ==
                  CURLM_CALL_MULTI_PERFORM);
            break;
        }
    } while(server->still_running&& (server->buffer_pos < want) );//
    return 1;
}

static void url_setopt(pHttpClientOps ops,int cmd,const char * parameters)
{
	  pHttpClientServer server =  ops;
	  if(server == NULL)
	  	return ;	  

	 if(server->http_handle == NULL )
	 {
	 	return ;
	 }
	 curl_easy_setopt(server->http_handle, cmd, parameters);

}

static int  url_readurl(pHttpClientOps ops,const char * url ,void * data,int len ,int timeout)
{

	url_setopt(ops,CURLOPT_URL,url);
	return url_read(ops,data,len,timeout);
}

static int url_post(pHttpClientServer ops,const char * url ,void * post,char *retdata ,int retdataLen)
{
	 	pHttpClientServer server = (pHttpClientServer) ops;
	  	if(server == NULL)
	  		return -1;
		
		char *contenttype =  "Content-type: application/json";
		CURL *curl;
		CURLcode res;
		struct curl_slist * headers= NULL;
	
			//www.baidu.com/#wd=java
		headers = curl_slist_append(headers,contenttype);
		curl_easy_setopt(server->http_handle, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(server->http_handle, CURLOPT_URL, url);
		
		curl_easy_setopt(server->http_handle, CURLOPT_POSTFIELDS, post);
		curl_easy_setopt(server->http_handle, CURLOPT_POSTFIELDSIZE, (long)strlen(post));
	  	curl_easy_setopt(server->http_handle, CURLOPT_POST, 1L);
	

		
	    return url_read(ops,retdata,retdataLen,10);

		/* Now specify we want to POST data */
		
}
//Content-type: application/json


static int url_read(pHttpClientOps ops,void * data,int len ,int timeout)
{
	
	  pHttpClientServer server = (pHttpClientServer) ops;
	  if(server == NULL)
	  	return -1;
	  //加入监听队列
	  curl_multi_add_handle(server->multi_handle,server->http_handle);
	
	  //设置超时
	  curl_easy_setopt(server->http_handle,CURLOPT_TIMEOUT,timeout);


	   curl_easy_setopt(server->http_handle, CURLOPT_VERBOSE, 0L);
       curl_easy_setopt(server->http_handle, CURLOPT_WRITEFUNCTION, write_callback);
	   curl_easy_setopt(server->http_handle, CURLOPT_WRITEDATA, server);
	   curl_multi_setopt(server->multi_handle, CURLMOPT_MAXCONNECTS, 1);

	  //请求
	  while(curl_multi_perform(server->multi_handle, &server->still_running) ==
              CURLM_CALL_MULTI_PERFORM );
	  
	  if((server->buffer_pos == 0) && (!server->still_running))
      {
            /* if still_running is 0 now, we should return NULL */
            /* make sure the easy handle is not in the multi handle anymore */
            curl_multi_remove_handle(server->multi_handle, server->http_handle);
			printf(" make sure the easy handle is not in the multi handle anymore \n");
			return -1;
      }
	  //等待接听
	  fill_buffer(server,len,timeout);

	  /* check if theres data in the buffer - if not fill_buffer()
	   * either errored or EOF */
	
	  if(!server->buffer_pos)
		  return 0;
	  /* ensure only available data is considered */
	  if(server->buffer_pos < len)
		  len = server->buffer_pos;
	  /* xfer data to caller */
	  memcpy(data, server->buffer, len);  
	  use_buffer(server,len);
            /* if still_running is 0 now, we should return NULL */
            /* make sure the easy handle is not in the multi handle anymore */
      curl_multi_remove_handle(server->multi_handle, server->http_handle);
	  return len;
	
}



//复制将数据写入文件的回调函数，关于回调函数，可以参考C程序设计伴侣8.5.4小节介绍
static size_t write_data(void * ptr,size_t size,size_t nmemb,FILE * stream)
{
    int written  = fwrite(ptr,size,nmemb,stream);
    return written;
}

static int download(char * url,char * outfile)  //char * url待下载文件的URL,char * out 下载后的文件名
{
    CURL * curl = NULL;
    FILE * fp = NULL;
    CURLcode res;
    //调用函数库中的curl_easy_init()函数完成初始化
    curl = curl_easy_init();
    if(curl)  //判断初始化是否成功
    {
        //以二进制可写方式打开文件，保存下载得到的数据
        fp = fopen(outfile,"wb");
        //根据libcurl的使用方法，设定下载的URL，写入函数以及写入的文件
        curl_easy_setopt(curl,CURLOPT_URL,url);
        curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,write_data);
        curl_easy_setopt(curl,CURLOPT_WRITEDATA,fp);
        //调用curl_easy_perform（）函数执行下载操作
        res = curl_easy_perform(curl);
        //下载完成，进行最后的清理工作
        curl_easy_cleanup(curl);
        //关闭文件
        fclose(fp);
        return res;
    }
    else
    {
        //如果初始化失败，返回相应的错误代码
        return -1;
    }
}



static size_t 
read_callback(char *buffer,
               size_t size,
               size_t nitems,
               void *userp)
{
	char *newbuff;
    int rembuff;
    pHttpClientServer url = (pHttpClientServer) userp;


	
    size *= nitems;
    rembuff=url->buffer_len - url->buffer_pos; /* remaining space in buffer */
    if(size > rembuff)
    {
        /* not enough space in buffer */
        newbuff=realloc(url->buffer,url->buffer_len + (size - rembuff));
        if(newbuff==NULL)
        {
            fprintf(stderr,"callback buffer grow failed\n");
            size=rembuff;
        }
        else
        {
            /* realloc suceeded increase buffer size*/
            url->buffer_len+=size - rembuff;
            url->buffer=newbuff;

            /*printf("Callback buffer grown to %d bytes\n",url->buffer_len);*/
        }
    }
    memcpy(&url->buffer[url->buffer_pos], buffer, size);
    url->buffer_pos += size;
	
    /*fprintf(stderr, "callback %d size bytes\n", size);*/
	
    return size;
}



static size_t 
write_callback(char *buffer,
               size_t size,
               size_t nitems,
               void *userp)
{
	char *newbuff;
    int rembuff;
    pHttpClientServer url = (pHttpClientServer) userp;

    size *= nitems;
    rembuff=url->buffer_len - url->buffer_pos; /* remaining space in buffer */
    if(size > rembuff)
    {
        /* not enough space in buffer */
        newbuff=realloc(url->buffer,url->buffer_len + (size - rembuff));
        if(newbuff==NULL)
        {
            fprintf(stderr,"callback buffer grow failed\n");
            size=rembuff;
        }
        else
        {
            /* realloc suceeded increase buffer size*/
            url->buffer_len+=size - rembuff;
            url->buffer=newbuff;

            /*printf("Callback buffer grown to %d bytes\n",url->buffer_len);*/
        }
    }
    memcpy(&url->buffer[url->buffer_pos], buffer, size);
    url->buffer_pos += size;
	
    /*fprintf(stderr, "callback %d size bytes\n", size);*/
    return size;
}

pHttpClientOps createHttpClientServer(void)
{

	pHttpClientServer server = (pHttpClientServer)malloc(sizeof(HttpClientServer));
	bzero(server,sizeof(HttpClientServer));
	
	if(server == NULL)
	{
		goto fail0;
	}
	server->http_handle = curl_easy_init();
	if(server->http_handle == NULL)
	{
		goto fail1;
	}
	server->multi_handle = curl_multi_init();
	if(server->multi_handle == NULL)
	{
		goto fail2;
	}


	 server->ops =  ops;
	 return server;
fail2:
		curl_easy_cleanup(server->http_handle);
		server->http_handle = NULL;
fail1:
	free(server);
fail0:
	return NULL;

}

void   destroyHttpClientServer(pHttpClientOps *ops)
{

	if(ops == NULL || *ops == NULL)
		return ;

	pHttpClientServer server  = (pHttpClientServer)*ops;
	if(server->multi_handle){
		
		curl_multi_cleanup(server->multi_handle);
		server->multi_handle = NULL;
	}
	if(server->http_handle){
		curl_easy_cleanup(server->http_handle);
		server->http_handle = NULL;
	}

	if(server->buffer_pos  >=0 )
    {
        /* ditch buffer - write will recreate */
        if(server->buffer)
            free(server->buffer);
        server->buffer=NULL;
        server->buffer_pos=0;
        server->buffer_len=0;
    }
	free(server);
	*ops = NULL;
}





























