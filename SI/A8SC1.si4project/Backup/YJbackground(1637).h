#ifndef _YJ_BACKGROUND__H_
#define _YJ_BACKGROUND__H_
#include "localDevice.h"


typedef enum T_RET_CODE{
	LOGON_TIMEOUT =  -2,
	LOGON_UNREGISTERED = -3,
};


typedef struct YJbackgroundOps{


	int (*login)(struct YJbackgroundOps*);
	int (*getDeviceInfo)(struct YJbackgroundOps*,pDeviceInfo);
	int (*getICcard)(struct YJbackgroundOps* ops,int pageno,pCardInfo cardPack,int pagesize,int *getsize);
	int (*upOpendoorRecord)(struct YJbackgroundOps*ops,char *idStr,int id ,char * cardType,char *status);
	int (*getUpdateRecord)(struct YJbackgroundOps*ops,int *verId,char *downUrl,int urlLen,char *md5,int md5Len);


}YJbackgroundOps,*pYJbackgroundOps;





pYJbackgroundOps createYJbackgroundServer(void);
void    destroyYJbackgroundServer(pYJbackgroundOps *server);


























#endif 
























