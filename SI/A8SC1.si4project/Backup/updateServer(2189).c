#include <unistd.h>   
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include "updateServer.h"
#include "common/Utils.h"
#include "common/debugLog.h"
#include "httpClient.h"


#define UPDATE_FILE "update.tar"

static 	int downloadAndUpdate(const char * url, const char *md5, int newVer );


static  UpdateServerOps ops = {
	.downloadAndUpdate = downloadAndUpdate,

};
pUpdateServerOps getUpdateServer(void)
{
	return &ops;
}
static 	int downloadAndUpdate(const char * url, const char *md5, int newVer )
{
	int  ret;
	char updateFileMd5[48] = {0};
	char systemCmd[128] = {0};
	int currentVersion;
	pHttpClientOps  httpServer =  getHttpClientServer();
	currentVersion = getUtilsOps->getAPPver(NULL);
	if(currentVersion >= newVer)
	{
		LOGW("It's the latest,don't have to update!\n");
		return -1;
	}
	ret = httpServer->download(url,UPDATE_FILE);
	if(ret  < 0 )
		return -1;
	if((access(UPDATE_FILE,F_OK)) <0) { 
		return -1;
	}	
	getUtilsOps()->md5FileTransform(UPDATE_FILE,updateFileMd5);
	printf("md5:%s\n",updateFileMd5);
	ret  = strcmp(updateFileMd5,md5);
	if(ret != 0)
	{
		remove(UPDATE_FILE);
		return -1;
	}
	sprintf(systemCmd,"tar -xjf  %s -C /usr/work/",UPDATE_FILE);
	system(systemCmd);
	if((access("/usr/work/update",F_OK)) <0) { 
		return -1;
	}
	system("cp /usr/work/update/*  / -R");
	getUtilsOps->setAPPver(newVer);
	return 0;
}	














































