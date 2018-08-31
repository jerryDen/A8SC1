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


	CURL *http_handle;
	CURLM *multi_handle;
	int still_running;
    char *buffer;               /* buffer to store cached data*/
    int buffer_len;             /* currently allocated buffers length */
    int buffer_pos;
}HttpClientServer,*pHttpClientServer;


static int download(const char * url,char * outfile);
static int getUrl(const char * url ,void * data,int len );
static int postJson(const char * url ,void * json,char *retdata ,int retdataLen);



static  int readHttpBuf( pHttpClientServer server,void * data,int len,int timeout);

static void  deleteServer(pHttpClientServer *server);
static pHttpClientServer newServer(void );




static size_t 
write_callback(char *buffer,
               size_t size,
               size_t nitems,
               void *userp);

static HttpClientOps  ops = {
			.getUrl = getUrl,
			.postJson = postJson,
			.download = download,
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




static int postJson(const char * url ,void * json,char *retdata ,int retdataLen)
{


		char *contenttype =  "Content-type: application/json";
		CURL *curl;
		CURLcode res;
		struct curl_slist * headers= NULL;
		int len = 0 ;
		pHttpClientServer server  = newServer();
	  	if(server ==NULL)
	  		goto fail0;
		
		
			//www.baidu.com/#wd=java
		headers = curl_slist_append(headers,contenttype);
		curl_easy_setopt(server->http_handle, CURLOPT_SSL_VERIFYPEER, 0L);//忽略证书检查
		curl_easy_setopt(server->http_handle, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(server->http_handle, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(server->http_handle, CURLOPT_URL, url);
		curl_easy_setopt(server->http_handle, CURLOPT_POSTFIELDS, json);
		curl_easy_setopt(server->http_handle, CURLOPT_POSTFIELDSIZE, (long)strlen(json));
	  	curl_easy_setopt(server->http_handle, CURLOPT_POST, 1L);



	    curl_easy_setopt(server->http_handle,CURLOPT_TIMEOUT,10);
	    curl_easy_setopt(server->http_handle, CURLOPT_VERBOSE, 0L);
        curl_easy_setopt(server->http_handle, CURLOPT_WRITEFUNCTION, write_callback);
	    curl_easy_setopt(server->http_handle, CURLOPT_WRITEDATA, server);
	    curl_multi_setopt(server->multi_handle, CURLMOPT_MAXCONNECTS, 1);

		
	
		len =  readHttpBuf(server,retdata,retdataLen,10);

		deleteServer(&server);
	    return len;
	
		fail0:
			return 0;


		/* Now specify we want to POST data */
		
}
//Content-type: application/json
static  int readHttpBuf( pHttpClientServer server,void * data,int len,int timeout)
{
		  //请求
	  int ret;
	 do{
	 		ret  = curl_multi_perform(server->multi_handle, &server->still_running);
	 } while( ret == CURLM_CALL_MULTI_PERFORM );

	  if((server->buffer_pos == 0) && (!server->still_running))
      {
            /* if still_running is 0 now, we should return NULL */
            /* make sure the easy handle is not in the multi handle anymore */
			printf("make sure the easy handle is not in the multi handle anymore  \
					buffer_pos:%d  still_running:%d\n",server->buffer_pos,server->still_running);
            curl_multi_remove_handle(server->multi_handle, server->http_handle);
			
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
static  int readHttpBufnew( pHttpClientServer server,void * data,int len,int timeout)
{
		  //请求
	int res;


	res = curl_easy_perform(server->http_handle);
	if (CURLE_OK != res){
			fprintf(stderr, "curl told us %d  !!\n", res);
			return 0;
	}
	  if(!server->buffer_pos)
		  return 0;
	  /* ensure only available data is considered */
	  if(server->buffer_pos < len)
		  len = server->buffer_pos;
	  /* xfer data to caller */
	  memcpy(data, server->buffer, len);  
	  use_buffer(server,len);
            /* if still_running is 0 now, we should return NULL */
         

	  return len;
	
}


static int  test(void)
{

	  CURL *curl;
	   CURLcode res;
	 
	   curl_global_init(CURL_GLOBAL_DEFAULT);
	 
	   curl = curl_easy_init();
	   if(curl) {
		 curl_easy_setopt(curl, CURLOPT_URL, "http://api.yunjiangkj.com/appVilla/login?");
	 
#ifndef SKIP_PEER_VERIFICATION
		 /*
		  * If you want to connect to a site who isn't using a certificate that is
		  * signed by one of the certs in the CA bundle you have, you can skip the
		  * verification of the server's certificate. This makes the connection
		  * A LOT LESS SECURE.
		  *
		  * If you have a CA cert for the server stored someplace else than in the
		  * default bundle, then the CURLOPT_CAPATH option might come handy for
		  * you.
		  */
		 curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif
	 
#ifndef SKIP_HOSTNAME_VERIFICATION
		 /*
		  * If the site you're connecting to uses a different host name that what
		  * they have mentioned in their server certificate's commonName (or
		  * subjectAltName) fields, libcurl will refuse to connect. You can skip
		  * this check, but this will make the connection less secure.
		  */
		 curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif
	 
		 /* Perform the request, res will get the return code */
		 res = curl_easy_perform(curl);
		 /* Check for errors */
		 if(res != CURLE_OK)
		   fprintf(stderr, "curl_easy_perform() failed: %s\n",
				   curl_easy_strerror(res));
	 
		 /* always cleanup */
		 curl_easy_cleanup(curl);
	   }
	 
	   curl_global_cleanup();
	 
	   return 0;

 }


static int getUrl(const char * url ,void * data,int len )
{
	
	  int retlen = 0;
	  pHttpClientServer server  = newServer();
	  if(server == NULL)
	  	goto fail0;
	   curl_global_init(CURL_GLOBAL_DEFAULT);

	   curl_easy_setopt(server->http_handle, CURLOPT_URL, url);
	   curl_easy_setopt(server->http_handle, CURLOPT_SSL_VERIFYPEER, 0L);//忽略证书检查
	   curl_easy_setopt(server->http_handle, CURLOPT_SSL_VERIFYHOST, 0L);
	   curl_easy_setopt(server->http_handle, CURLOPT_TIMEOUT,10);
	    
	   curl_easy_setopt(server->http_handle, CURLOPT_VERBOSE, 0L);
       curl_easy_setopt(server->http_handle, CURLOPT_WRITEFUNCTION, write_callback);
	   curl_easy_setopt(server->http_handle, CURLOPT_WRITEDATA, server);
	   
	 //curl_multi_setopt(server->multi_handle, CURLMOPT_MAXCONNECTS, 1);
	   retlen  = readHttpBufnew(server,data,len,10);
	   
	   deleteServer(&server);
	   return retlen;
fail0:
	return -1;
	
}



//复制将数据写入文件的回调函数，关于回调函数，可以参考C程序设计伴侣8.5.4小节介绍
static size_t write_data(void * ptr,size_t size,size_t nmemb,FILE * stream)
{
    int written  = fwrite(ptr,size,nmemb,stream);
    return written;
}

static int download(const char * url,char * outfile)  
{
    CURL * curl = NULL;
    FILE * fp = NULL;
    CURLcode res;
    curl = curl_easy_init();
    if(curl != NULL)  
    {
        fp = fopen(outfile,"wb");
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);//忽略证书检查
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl,CURLOPT_URL,url);
        curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,write_data);
        curl_easy_setopt(curl,CURLOPT_WRITEDATA,fp);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
		printf("res == %d\n",res);
        fclose(fp);
        return res;
    }
    else
    {
        return -1;
    }
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
	printf("write_callback!\n");
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
    }else {

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

pHttpClientOps getHttpClientServer(void)
{
	return &ops;
}


static pHttpClientServer newServer(void )
{
	  int ret;
	  pHttpClientServer server = (pHttpClientServer)malloc(sizeof(HttpClientServer));
	  bzero( server ,sizeof(HttpClientServer));
	  
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
	  if( server->multi_handle == NULL)
	  {
		  goto fail2;
	  }
	  ret  = curl_multi_add_handle(server->multi_handle,server->http_handle);
	 
	  return server;
	  fail2:
			  curl_easy_cleanup(server->http_handle);
			  server->http_handle = NULL;
	  fail1:
		  free(server);
	  fail0:
		  return NULL;	
}

static void  deleteServer(pHttpClientServer *pserver)
{

	if(pserver == NULL || *pserver == NULL)
		return ;

	pHttpClientServer server  = *pserver;
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
	*pserver = NULL;
}





























