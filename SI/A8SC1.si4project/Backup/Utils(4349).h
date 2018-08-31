

#ifndef UTILS_H_
#define UTILS_H_


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
	unsigned char (*NByteCrc8)(unsigned char , unsigned char *, unsigned int DatLn);
	void (*YUYVToNV21)(const void* , void *, int , int );
	void (*YUYVToNV12)(const void* , void *, int , int );
	int  (*charToInt)(char );
	void (*printData)(char *, int );
	void (*printHex)(char *,int);
	int (*GetWeiGendCardId)(const char *,int , int *);
	int (*getHardWareVer)(char* ,int );
	int (*getHardWareFromRK)(char *,int);
	int (*getCpuVer)(void);
}UtilsOps,*pUtilsOps;

pUtilsOps getUtilsOps(void);







#endif /* UTILS_H_ */
