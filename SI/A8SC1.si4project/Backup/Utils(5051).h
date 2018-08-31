#ifndef UTILS_H_
#define UTILS_H_

#include "common/cJSON.h"

#define false -1
#define true 0
#ifndef DENF_CPU_VER
#define DENF_CPU_VER

typedef enum {
	A20 = 0,
	A64,
	RK3368,
}CPU_VER;
#endif

typedef struct {
	unsigned char (*ByteCrc8)(unsigned char , unsigned char );
	unsigned char (*NByteCrc8)(unsigned char , unsigned char *, unsigned int );
	void (*YUYVToNV21)(const void* , void *, int , int );
	void (*YUYVToNV12)(const void* , void *, int , int );
	int  (*charToInt)(char );
	void (*printData)(char *, int );
	void (*printHex)(char *,int);
	 int (*GetWeiGendCardId)(unsigned char* rawData,int rawDatalen,char* validData,int validLen);
	int (*getHardWareVer)(char* ,int );
	int (*getHardWareFromRK)(char *,int);
	int (*getCpuVer)(void);
	char *(*md5StrTransform)(const char *, char *);
	char * (*md5FileTransform)(const char *fileName, char *d);
    int (*getStrFormCjson)(const char *,const char *,char *,int);
	int  (*getIntFormCjson)(const char *,const char *,int *value);
	int (*getChildDoubleFormCjson)(const char *,const char *,const char *,double *);
	int (*getChildIntFormCjson)(const char *,const char *,const char *,int *);
	int (*getChildStrFormCjson)(const char *,const char *,const char *,char *,int );
    int  (*getDoubleFormCjson)(const char *,const char * ,double *value);
	int (*getMacAddress)(char *);
	unsigned long long   (*getTimestamp)(char *);
	unsigned long long  (*getTimeTick)(const char *str_time);
	int (*getAPPver)(char *);
}UtilsOps,*pUtilsOps;

pUtilsOps getUtilsOps(void);







#endif /* UTILS_H_ */
