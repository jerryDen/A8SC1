
#ifndef    __UPDATE_SERVER__H__
#define    __UPDATE_SERVER__H__






typedef struct UpdateServerOps{

	int (*downloadAndCheck)(const char * url, const char *md5); 
	int (*update)(void);
}UpdateServerOps,*pUpdateServerOps;


pUpdateServerOps getUpdateServer(void);







#endif































