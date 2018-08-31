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

static int  httpsUrlToHttpUrl(const char * s ,char * d ,int len)
{
	char *find;
	find = strstr(s,"https");
	if(find == NULL){
		if(len < strlen(s))
			return -1;
		strcpy(d,s);
		return 0;
	}
	if(len < strlen(find))
		return -1;
	sprintf(d,"http%s",find+5);
	return 0;	
}

static 	int downloadAndUpdate(const char * url, const char *md5, int newVer )
{
	int  ret;
	char updateFileMd5[48] = {0};
	char systemCmd[128] = {0};
	char httpUrl[1024] = {0};
	int currentVersion;
	pHttpClientOps  httpServer =  getHttpClientServer();
	
	httpsUrlToHttpUrl(url,httpUrl,sizeof(httpUrl));

	
	currentVersion = getUtilsOps()->getAPPver(NULL);
	if(currentVersion >= newVer)
	{
		LOGW("It's the latest,don't have to update!\n");
		return -1;
	}
	ret = httpServer->download(httpUrl,UPDATE_FILE);
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
	sprintf(systemCmd,"unzip -o  %s -d /usr/work/",UPDATE_FILE);
	system(systemCmd);
	if((access("/usr/work/update",F_OK)) <0) { 
		return -1;
	}
	system("chmod 755 /usr/work/update/* -R");
	system("cp /usr/work/update/*  / -R");
	
	system("rm -rf /usr/work/update");
	getUtilsOps()->setAPPver(newVer);
	system("reboot\n");
	return 0;
}	














































