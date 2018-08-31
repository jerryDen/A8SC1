#include <unistd.h>   
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "updateServer.h"
#include "common/Utils.h"
#include "common/debugLog.h"
#include "httpClient.h"


#define UPDATE_FILE "update.tar"

static 	int downloadAndCheck(const char * url, const char *md5); 
static	int update(void);

 static  UpdateServerOps ops = {
	.downloadAndCheck = downloadAndCheck,
	.update = update,
};
pUpdateServerOps getUpdateServer(void)
{
	return &ops;
}

static 	int downloadAndCheck(const char * url, const char *md5)
{
	int ret;
	char updateFileMd5[48] = {0};
	pHttpClientOps   httpServer =  getHttpClientServer();
	ret  = httpServer->download(url,UPDATE_FILE);
	if(ret  < 0 )
		return -1;
	getUtilsOps()->md5FileTransform(UPDATE_FILE,updateFileMd5);
	printf("md5:%s\n",updateFileMd5);
	ret  = strcmp(updateFileMd5,md5);
	if(ret != 0)
	{
		if((access(UPDATE_FILE,F_OK)) != -1) { 
			remove(UPDATE_FILE);
			return -1;
		}
	}
	return 0;
}	

static	int update(void)
{
	if((access(UPDATE_FILE,F_OK)) <0) { 
		
			return -1;
	}
	char cmd[128] = {0};
	
	sprintf(cmd,"tar -xjf  %s -C /usr/work/",UPDATE_FILE);
	system(cmd);

	if((access("/usr/work/update",F_OK)) <0) { 
		
			return -1;
	}
	system("cp /usr/work/update/*  / -R");

	
	
	return 0;	
	
}












































