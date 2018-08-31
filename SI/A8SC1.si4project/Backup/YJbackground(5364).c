#include <stdio.h>
#include <stdlib.h>
#include "YJbackground.h"

#include "common/Utils.h"
#include "httpClient.h"
#include "localDevice.h"

#define LICENSE  "ed98aABEWQ#&9I0@QE"

typedef struct YJbackgroundServer{
		YJbackgroundOps ops;
		pHttpClientOps httpClient;
		DeviceInfo device;
		int isLoginSucceed;
	


}YJbackgroundServer,*pYJbackgroundServer;


static  int login(struct YJbackgroundOps* ops);
static 	int getICcard(struct YJbackgroundOps* ops,int pageno,pCardInfo cardPack,int pagesize,int *getsize);


static	int upOpendoorRecord(struct YJbackgroundOps*ops,char *idStr,int id ,char * cardType,char *status);

static	int getUpdateRecord(struct YJbackgroundOps*ops,int *verId,char *downUrl,int urlLen,char *md5,int md5Len);
static  int getDeviceInfo(struct YJbackgroundOps* ops,pDeviceInfo deviceInfo);

static void getfkey(const char *value,const char *timestamp,char *fkey);


YJbackgroundOps ops = {
	.login = login,
	.getICcard = getICcard,
	.upOpendoorRecord = upOpendoorRecord,
	.getUpdateRecord = getUpdateRecord,
	.getDeviceInfo = getDeviceInfo,
};


//MD5(指定参数value值+时间戳（1482199108451）+密钥)

//?FKEY=c1ae3ec28b8fd371dc3738b514f2dfce0&TIMESTAMP=1533723497000&LOCKMAC=00:0c:29:d3:d6:db&VERSION=1000"




static void getfkey(const char *value,const char *timestamp,char *fkey)
{

		char  md5sourceStr[64] = {0};
		sprintf(md5sourceStr,"%s%s%s",value,timestamp,LICENSE);
		getUtilsOps()->md5StrTransform(md5sourceStr,fkey);
}

static  int login(struct YJbackgroundOps* ops)
{
	pYJbackgroundServer server =  (pYJbackgroundServer)ops;
	if(server == NULL)
		return -1;
	char *url  = "https://api.yunjiangkj.com/appVilla/login?";
	char  appver[12] = {0};
	char  md5sourceStr[64] = {0};
	char  macaddres[36] = {0};
	char  timestamp[24] = {0};
	char  fkey[48] = {0};
	char  getparameter[256] = {0};
	char  jsonStr[256]= {0};
	char  httpCode[12] = {0};

	int  lockid;
	char deviceDame[64];
	int  communityId;
	int  ret = -1;	
	getUtilsOps()->getAPPver(appver);
	getUtilsOps()->getTimestamp(timestamp);
	getUtilsOps()->getMacAddress(macaddres);
	getfkey(macaddres,timestamp,fkey);
	sprintf(getparameter,"%sFKEY=%s&TIMESTAMP=%s&LOCKMAC=%s&VERSION=%s",url,fkey,timestamp,macaddres,appver);
	ret = server->httpClient->readurl(server->httpClient,getparameter,jsonStr,sizeof(jsonStr)-1,10);
	if(ret < 0)
	{
		return LOGON_TIMEOUT;
	}
	printf("jsonStr:%s\n ",jsonStr);
	ret  = getUtilsOps()->getStrFormCjson(jsonStr,"code",httpCode,sizeof(httpCode));
	if(ret < 0)
	{
		return -3;
	}
	ret  = strcmp("101",httpCode);
	if(ret != 0 )
	{
		
		return LOGON_UNREGISTERED;
	}
	server->isLoginSucceed = 1;
	ret  = getUtilsOps()->getChildIntFormCjson(jsonStr,"data","LOCKID",&lockid);
	if(ret == 0)
	{
		server->device.lockid = lockid;
	}
	ret  = getUtilsOps()->getChildIntFormCjson(jsonStr,"data","COMMUNITYID",&communityId);
	if(ret == 0)
	{
		server->device.communityId = communityId;
	}

	ret  = getUtilsOps()->getChildStrFormCjson(jsonStr,"data","LOCKNAME",deviceDame,sizeof(deviceDame));
	if(ret > 0)
	{
		strcpy(server->device.deviceDame,deviceDame);
	}
	

	return 0;

}




static  int  getDeviceInfo(struct YJbackgroundOps* ops,pDeviceInfo deviceInfo)
{

	pYJbackgroundServer server =  (pYJbackgroundServer)ops;
	if(server == NULL||deviceInfo == NULL)
		return -1;
	if(server->isLoginSucceed){

		*deviceInfo = server->device;
	}
	return -1;
}
static int getTotalPage(const char *jsonStr)
{
	int totalPage  = 0;
	int ret;
	char code[12] = {0};
	
	ret = getUtilsOps()->getChildIntFormCjson(jsonStr,"data","TOTALPAGE",&totalPage);
	if(ret < 0)
		return ret;
	return totalPage;
	
}

static 	int getICcard(struct YJbackgroundOps* ops,int pageno,pCardInfo cardPack,int pagesize,int *getsize)
{
	pYJbackgroundServer server =  (pYJbackgroundServer)ops;
	if(server == NULL)
		return -1;
	char *url  = "http://api.yunjiangkj.com/appVilla/getAccessCard.do?";
	char  timestamp[24] = {0};
	char  fkey[48] = {0};
	char  getparameter[256] = {0};
	char  jsonStr[1024*100]= {0};
	char  httpCode[12] = {0};
	char  lockid[24] = {0};
	int   ret;
	int   totalPage;
	getUtilsOps()->getTimestamp(timestamp);
	
	sprintf(lockid,"%d",server->device.lockid);
	getfkey(lockid,timestamp,fkey);
	
	sprintf(getparameter,"%sFKEY=%s&TIMESTAMP=%s&LOCKID=%d&COMMUNITYID=%d&PAGENO=%d&PAGESIZE=%d",url,fkey,timestamp,server->device.lockid,server->device.communityId,pageno,pagesize);
	ret = server->httpClient->readurl(server->httpClient,getparameter,jsonStr,sizeof(jsonStr)-1,10);
	if(ret < 0)
	{
		return LOGON_TIMEOUT;
	}
	printf("jsonStr = %s\n",jsonStr);
	totalPage = getTotalPage(jsonStr);
	if(totalPage <= 0)
		return 0;


	//获取数组头
	cJSON *jsonroot = cJSON_Parse(jsonStr); 
	cJSON *child = cJSON_GetObjectItem(jsonroot,"data");
	cJSON *listArray = cJSON_GetObjectItem(child,"LIST");

	int arrySize=cJSON_GetArraySize(listArray);

	printf("arrySize = %d\n",arrySize);
	
	*getsize = arrySize>pagesize?pagesize:arrySize;
	
	cJSON *listhead = listArray->child;
	while(listhead && (pagesize > 0) ){
		cJSON *child;
		child = cJSON_GetObjectItem(listhead,"DISABLEDATE");
		if(child != NULL)
		{
			 int strLen = strlen(child->valuestring);
			 int spaceLen = sizeof(cardPack->disabledate)-1;
			 int cpl = strLen>spaceLen?spaceLen:strLen;
			 strncpy(cardPack->disabledate,child->valuestring,cpl);
			
		}
		child = cJSON_GetObjectItem(listhead,"UNITID");
		if(child != NULL)
		{
			 cardPack->untitd = child->valueint;
			 
		}
		child = cJSON_GetObjectItem(listhead,"STATE");
		if(child != NULL)
		{
			 int strLen = strlen(child->valuestring);
			 int spaceLen = sizeof(cardPack->state)-1;
			 int cpl = strLen>spaceLen?spaceLen:strLen;
			 strncpy(cardPack->state,child->valuestring,cpl);
		}
		child = cJSON_GetObjectItem(listhead,"CELLID");
		if(child != NULL)
		{
			 cardPack->cellid = child->valueint;
			 
		}
		child = cJSON_GetObjectItem(listhead,"RID");
		if(child != NULL)
		{
			 cardPack->rid = child->valueint;
			 
		}
		child = cJSON_GetObjectItem(listhead,"BLOCKID");
		if(child != NULL)
		{
			 cardPack->blockid = child->valueint;
			 
		}

		child = cJSON_GetObjectItem(listhead,"TYPE");
		if(child != NULL)
		{
			
			 int strLen = strlen(child->valuestring);
			 int spaceLen = sizeof(cardPack->type)-1;
			 int cpl = strLen>spaceLen?spaceLen:strLen;
			 strncpy(cardPack->type,child->valuestring,cpl);
		}
		
		child = cJSON_GetObjectItem(listhead,"DISTRICTID");
		if(child != NULL)
		{
			 cardPack->districtid = child->valueint;
			 
		}

		child = cJSON_GetObjectItem(listhead,"CARDNO");
		if(child != NULL)
		{
			 int strLen = strlen(child->valuestring);
			 int spaceLen = sizeof(cardPack->cardid)-1;
			 int cpl = strLen>spaceLen?spaceLen:strLen;
			 strncpy(cardPack->cardid,child->valuestring,cpl);
		}
	
		listhead = listhead->next;
		cardPack += 1;
		pagesize --;

	}
	return totalPage;

}
static	int upOpendoorRecord(struct YJbackgroundOps*ops,char *idStr,int id ,char * cardType,char *status)
{
	pYJbackgroundServer server =  (pYJbackgroundServer)ops;
	if(server == NULL)
			return -1;
	
				
	char *url  = "http://api.yunjiangkj.com/appVilla/uploadAccess?";
	double   timestamp;
	char  fkey[48] = {0};
	char  getparameter[256] = {0};
	char  jsonStr[1024*10]= {0};
	char  httpCode[12] = {0};
	char  lockid[24] = {0};
	char  timestampStr[24] = {0};
	char* postJson; 

	sprintf(lockid,"%u",server->device.lockid);
	timestamp = getUtilsOps()->getTimestamp(timestampStr);
	
	getfkey(lockid,timestampStr,fkey);
		
	cJSON* pRoot = cJSON_CreateObject();  
    cJSON* pArray = cJSON_CreateArray();
	cJSON* pItem = cJSON_CreateObject();
	cJSON* getcontent = NULL;
	
	cJSON_AddStringToObject(pRoot, "FKEY", fkey); 
	cJSON_AddNumberToObject(pRoot,"TIMESTAMP",timestamp);

	cJSON_AddNumberToObject(pRoot,"COMMUNITYID",server->device.communityId);
	cJSON_AddNumberToObject(pRoot, "LOCKID", server->device.lockid); 
	
	cJSON_AddItemToObject(pRoot, "LIST", pArray);
	cJSON_AddStringToObject(pItem, "TYPE", cardType);  
	cJSON_AddStringToObject(pItem, "KEYINFO", idStr); 
    cJSON_AddNumberToObject(pItem, "KEYID", id); 
	cJSON_AddStringToObject(pItem, "STATUS", status);
	cJSON_AddNumberToObject(pItem, "OPERATETIME", timestamp); 
	
    cJSON_AddItemToArray(pArray, pItem);
		
	postJson = cJSON_Print(pRoot);
	printf("postJson:%s\n",postJson);
	server->httpClient->urlpost(server->httpClient,url,postJson,jsonStr,sizeof(jsonStr));
	
	printf("json:%s\n",jsonStr);
	

//	curl -l -H "Content-type: application/json" -X POST -d '{"phone":"13521389587","password":"test"}' http://api.yunjiangkj.com/appVilla/uploadAccess


	
}
static	int getUpdateRecord(struct YJbackgroundOps*ops,int *verId,char *downUrl,int urlLen,char *md5,int md5Len)
{

		pYJbackgroundServer server =  (pYJbackgroundServer)ops;
		if(server == NULL)
			return -1;

		char *url  = "http://api.yunjiangkj.com/appVilla/upgrade?";
		char  appver[12] = {0};
		char  md5sourceStr[64] = {0};
		char  macaddres[36] = {0};
		char  timestamp[24] = {0};
		char  fkey[48] = {0};
		char  getparameter[256] = {0};
		char  jsonStr[256]= {0};
		char  httpCode[12] = {0};
		int   ret;

		getUtilsOps()->getAPPver(appver);
		getUtilsOps()->getTimestamp(timestamp);
		getUtilsOps()->getMacAddress(macaddres);
		getfkey(macaddres,timestamp,fkey);
		sprintf(getparameter,"%sFKEY=%s&TIMESTAMP=%s&LOCKMAC=%s&VERSION=%s",url,fkey,timestamp,macaddres,appver);
		ret = server->httpClient->readurl(server->httpClient,getparameter,jsonStr,sizeof(jsonStr)-1,10);
		if(ret < 0)
		{
			return LOGON_TIMEOUT;
		}
		printf("jsonStr:%s\n ",jsonStr);
		ret  = getUtilsOps()->getStrFormCjson(jsonStr,"code",httpCode,sizeof(httpCode));
		if(ret < 0)
		{
			return -3;
		}
		ret  = strcmp("101",httpCode);
		if(ret != 0 )
		{
			
			return LOGON_UNREGISTERED;
		}

		ret  = getUtilsOps()->getChildIntFormCjson(jsonStr,"data","VERSION",&verId);
		if(ret != 0)
		{
			return -1;
		}
		ret  = getUtilsOps()->getChildStrFormCjson(jsonStr,"data","MD5",md5,md5Len);
		if(ret != 0)
		{
			return -1;
		}

		ret  = getUtilsOps()->getChildStrFormCjson(jsonStr,"data","URL",downUrl,urlLen);
		if(ret <=0 )
		{
			return -1;
		}

		return 0;	
}

pYJbackgroundOps createYJbackgroundServer(void)
{
	pYJbackgroundServer server = (pYJbackgroundServer)malloc(sizeof(YJbackgroundServer));
	if(server ==  NULL)
		goto fail0;

	server->httpClient = createHttpClientServer();
	if(server->httpClient == NULL)
	{
		goto fail1;
	}

	server->ops = ops;
	return server;
	fail1:
		free(server);
		server = NULL;
	fail0:
		return NULL;

}





void    destroyYJbackgroundServer(pYJbackgroundOps *pthis)
{
	if(pthis == NULL || (pthis = NULL))
		return ;
	pYJbackgroundServer server = (pYJbackgroundServer)(*pthis);
	
	destroyHttpClientServer(&server->httpClient);


}



