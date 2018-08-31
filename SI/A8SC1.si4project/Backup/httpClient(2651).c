

#include "httpClient.h"


typedef struct HttpClientServer{

	HttpClientOps  ops;
	CURL *http_handle;
	CURLM *multi_handle;

	int pipes[2] 


}HttpClientServer,*pHttpClientServer;



static size_t 
write_callback(char *buffer,
               size_t size,
               size_t nitems,
               void *userp)
{



	



}





pHttpClientOps createHttpClientServer(void)
{

	pHttpClientServer server = (pHttpClientServer)malloc(sizeof(HttpClientServer));
	if(server == NULL)
	{
		goto fail0:
	}

	
	server->http_handle = curl_easy_init();
	if(server->http_handle == NULL)
	{
		goto fail1;
	}
	
	

		
fail1:
	free(server);
fail0:
	return NULL;

}

void   destroyHttpClientServer(pHttpClientOps *ops)
{

	






}





























