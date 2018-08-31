
#ifndef    __UPDATE_SERVER__H__
#define    __UPDATE_SERVER__H__






typedef struct UpdateServerOps{

	int (*downloadAndUpdate)(const char * url, const char *md5, int newVer );

}UpdateServerOps,*pUpdateServerOps;

pUpdateServerOps getUpdateServer(void);







#endif































