#include <stdio.h>
#include "httpClient.h"
#include "md5.h"
#include "common/Utils.h"


int main(void)
{
	char buffer[1024] = { 0};
	int ret = 0;
	//MD5(000c29d3d6db1533723497000ed98aABEWQ#&9I0@QE)
	//MAC = 00:0c:29:d3:d6:db
	//time=1533723497000
	//ver=1000

	
	char *url  = "https://api.yunjiangkj.com/appVilla/login?FKEY=cae3ec28b8fd371dc3738b514f2dfce0&TIMESTAMP=1533723497000&LOCKMAC=00:0c:29:d3:d6:db&VERSION=1000";
	pHttpClientOps httpClass = createHttpClientServer();
	if(httpClass == NULL)
	{
		return -1;
	}
	ret = httpClass->readurl(httpClass,url,buffer,sizeof(buffer)-1,10);
	if(ret > 0)
	{
		printf("%s\n",buffer);
	}
	printf("\nlen = %d\n",ret);
    destroyHttpClientServer(&httpClass);


	char md5[32]  ={0};
	getUtilsOps()->md5StrTransform("admin",md5);
	printf("%s\n",md5);

	
	return 0;
}






















