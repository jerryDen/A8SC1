/*
 * Utils.cpp
 *
 *  Created on: 2015-1-26
 *      Author: Jerry
 */

#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/utsname.h>
#include <limits.h>
#include <ctype.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <net/route.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include "md5.h"

#include "Utils.h"
#include "DebugLog.h"

#define DEBUGLEVEL 7

#define ETHTOOL_GLINK  0x0000000a /* Get link status (ethtool_value) */

struct ethtool_value {
	unsigned int cmd;
	unsigned int data;
};
static ssize_t  myGetline(char **lineptr, ssize_t *n, FILE *stream);
//控制通讯须带效验
static unsigned char CRC8_TABLE[] = { 0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65, 157, 195, 33, 127, 252, 162, 64, 30, 95, 1, 227, 189, 62, 96, 130,
		220, 35, 130, 159, 193, 66, 28, 254, 160, 225, 191, 93, 3, 128, 222, 60, 98, 190, 224, 2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255, 70, 24, 250, 164, 39,
		121, 155, 197, 132, 218, 56, 102, 229, 187, 89, 7, 219, 133, 103, 57, 186, 228, 6, 88, 25, 71, 165, 251, 120, 38, 196, 154, 101, 59, 217, 135, 4, 90, 184, 230, 167, 249,
		27, 69, 198, 152, 122, 36, 248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91, 5, 231, 185, 140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147,
		205, 17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80, 175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238, 50, 108, 142, 208,
		83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115, 202, 148, 118, 40, 171, 245, 23, 73, 8, 86, 180, 234, 105, 55, 213, 139, 87, 9, 235, 181, 54, 104, 138, 212, 149,
		203, 41, 119, 244, 170, 72, 22, 233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168, 116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137,
		107, 53 };

//////////单字节数据CRC8计算/////////////////////
static unsigned char ByteCrc8(unsigned char Crc8Out, unsigned char Crc8in) {
	return CRC8_TABLE[Crc8in ^ Crc8Out];
}

////////////多字节数据CRC8计算////////////////////
static unsigned char NByteCrc8(unsigned char crc_in, unsigned char *DatAdr, unsigned int DatLn) {
	unsigned char Crc8Out;

	Crc8Out = crc_in;
	while (DatLn != 0) {
		Crc8Out = CRC8_TABLE[*DatAdr ^ Crc8Out];
		DatAdr++;
		DatLn--;
	}
	return Crc8Out;
}

static void YUYVToNV12(const void* yuyv, void *nv12, int width, int height) {
	int i, j;
	uint8_t* Y = (uint8_t*) nv12;
	uint8_t* UV = (uint8_t*) Y + width * height;

	for (i = 0; i < height; i += 2) {
		for (j = 0; j < width; j++) {
			*(uint8_t*) ((uint8_t*) Y + i * width + j) = *(uint8_t*) ((uint8_t*) yuyv + i * width * 2 + j * 2);
			*(uint8_t*) ((uint8_t*) Y + (i + 1) * width + j) = *(uint8_t*) ((uint8_t*) yuyv + (i + 1) * width * 2 + j * 2);
			*(uint8_t*) ((uint8_t*) UV + ((i * width) >> 1) + j) = *(uint8_t*) ((uint8_t*) yuyv + i * width * 2 + j * 2 + 1);
		}
	}
}
static int GetWeiGendCardId(const char *hexIn,int len, int *id)
 {
 	char s[6];
 	if(hexIn == NULL||len >6 )
 		return -1;
 	union {
 		char buf[sizeof(uint32_t)];
 		uint32_t id;
 	}intChar_union;
 	union {
 		char buf[sizeof(uint16_t)];
 		uint16_t id;
 	}shortChar_union;
 	sprintf(s, "%03u", hexIn[2]);
 	intChar_union.buf[0]  = (s[0] & 0x0f) << 4;
 	intChar_union.buf[0] |= (s[1] & 0x0f);
 	intChar_union.buf[1]  = (s[2] & 0x0f) << 4;
 	memcpy(shortChar_union.buf, hexIn, sizeof(uint16_t));
 	sprintf(s, "%05u", shortChar_union.id);
 	intChar_union.buf[1] |= (s[0] & 0x0f);
 	intChar_union.buf[2]  = (s[1] & 0x0f) << 4;
 	intChar_union.buf[2] |= (s[2] & 0x0f);
 	intChar_union.buf[3]  = (s[3] & 0x0f) << 4;
 	intChar_union.buf[3] |= (s[4] & 0x0f);
 	*id = intChar_union.id;
 	return len;
 }
static void YUYVToNV21(const void* yuyv, void *nv21, int width, int height) {
	int i, j;
	uint8_t* Y = (uint8_t*) nv21;
	uint8_t* VU = (uint8_t*) Y + width * height;

	for (i = 0; i < height; i += 2) {
		for (j = 0; j < width; j++) {
			*(uint8_t*) ((uint8_t*) Y + i * width + j) = *(uint8_t*) ((uint8_t*) yuyv + i * width * 2 + j * 2);
			*(uint8_t*) ((uint8_t*) Y + (i + 1) * width + j) = *(uint8_t*) ((uint8_t*) yuyv + (i + 1) * width * 2 + j * 2);

			if (j % 2) {
				if (j < width - 1) {
					*(uint8_t*) ((uint8_t*) VU + ((i * width) >> 1) + j) = *(uint8_t*) ((uint8_t*) yuyv + i * width * 2 + (j + 1) * 2 + 1);
				}
			} else {
				if (j > 1) {
					*(uint8_t*) ((uint8_t*) VU + ((i * width) >> 1) + j) = *(uint8_t*) ((uint8_t*) yuyv + i * width * 2 + (j - 1) * 2 + 1);
				}
			}
		}
	}
}

/*************************************************
 Function: char_to_int
 Description: 字符型转化为整型
 Input:  s :要转化字符
 Output:
 Return: 成功: 返回转化后整型数;
 失败:返回-1 ;
 Others:
 *************************************************/
static int charToInt(char s) {
	if (s >= '0' && s <= '9') {
		return (s - '0');
	} else if (s >= 'A' && s <= 'F') {
		return (s - 'A' + 10);
	} else if (s >= 'a' && s <= 'f') {
		return (s - 'a' + 10);
	}

	return -1;
}
static void printHex(char *buf,int len)
{
	int i,j = 0,c;
	char *buffer = malloc(len*7*8+1);
	char temp;
	bzero(buffer,len*7*8+1);
	for(i = 0; i<len;i++)
	{
		temp = buf[i];
		for(c=0;c<8;c++ )
		{
			j += sprintf(&buffer[j],"%d ",(temp&0x80)>>7 );
			temp <<= 1;
		}
		j += sprintf(&buffer[j],"%s","\n");
	}
	LOGD("%s\n",buffer);
	free(buffer);
}
static void printData(char *buf, int len) {
	int i,j = 0;
	char *buffer = malloc(len*7+1);
	bzero(buffer,len*7+1);
	for(i = 0; i<len;i++)
	{
		j += sprintf(&buffer[j],"0x%-2x ",buf[i]);
		
	}
	LOGD("%s",buffer);
	free(buffer);
}
static int getKey(FILE *fp, char *p, char* getBuff,int bufflen,char  mark) {
	char Buff[100] = { '\0' };
	char *line = NULL;
	char *equalFlag = NULL;
	int bufLen = strlen(p);
	int lineLen = 0;
	int i = 0;
	ssize_t len = 0;

	while ((lineLen = myGetline(&line, &len, fp) )> 0) {

		if(bufLen >= lineLen)
			continue;

		if (strncmp(line, p, bufLen) == 0) {
			equalFlag = strchr(line, mark);
			if (equalFlag) {

				if((strlen(equalFlag)+1) >= bufflen )
					return -1;
				strcpy(getBuff, equalFlag + 1);
				if(line  != NULL)
					free(line);
				line = NULL;
				len = 0;
				return strlen(getBuff);
			}
		}
		if(line  != NULL)
			free(line);
		line = NULL;
		len = 0;
	}

	if(line  != NULL)
		free(line);
	return -1;
}
static ssize_t  myGetline(char **lineptr, ssize_t *n, FILE *stream)
{
    ssize_t count=0;
    int buf;

    if(*lineptr == NULL)
    {
        *n = 120;
        *lineptr = malloc(*n);
    }
    while((buf = fgetc(stream))!=EOF)
    {
        if(buf == '\n')
        {
             count += 1;
                  break;
        }
        count++;
        *(*lineptr+count-1) = buf;
        *(*lineptr+count) = '\0';
        if(*n <= count)                    //重新申请内存
        	*lineptr = realloc(*lineptr,count*2);
    }
    return count;
}
static int getHardWareFromRK(char* buf,int len)
{
	int ret;
	FILE * fp = fopen("/system/build.prop", "r");
	if (fp == NULL){
		LOGE("OPEN /system/build.prop FAIL !\n");
		return -1;
	}
	ret  = getKey(fp,"ro.rksdk.version",buf,len,'=');
	fclose(fp);
	return ret;

}
static int getHardWareVer(char* buf,int len)
{

	int ret;
	FILE * fp = fopen("/system/build.prop", "r");
	if (fp == NULL){
		LOGE("OPEN /system/build.prop FAIL !\n");
		return -1;
	}
	ret  = getKey(fp,"ro.product.firmware",buf,len,'=');
	fclose(fp);
	return ret;
}
static int getCpuVer(void)
{
#define A20_CPU 	" sun7i"
#define A64_CPU 	" sun50iw1p1"
#define RK3368_CPU  " rockchip,rk3368"
	int ret;
	char buf[128] = {0};
	FILE * fp = fopen("/proc/cpuinfo", "r");
	if (fp == NULL){
		LOGE("OPEN /proc/cpuinfo FAIL !\n");
		return -1;
	}
	ret  = getKey(fp,"Hardware",buf,sizeof(buf),':');


	fclose(fp);
	if(strcmp(buf,A20_CPU)==0)
	{
		return A20;
	}else if(strcmp(buf,A64_CPU)==0)
		return A64;
	else  if(strcmp(buf,RK3368_CPU) == 0 )
		return RK3368;
	return 0;
}


void HexToStr(char *pbDest, unsigned char *pbSrc, int nLen)
{
	char	ddl,ddh;	
	int i;
	for (i = 0; i<nLen; i++)
	{
		ddh = 48 + pbSrc[i] / 16;
		ddl = 48 + pbSrc[i] % 16;  
		if (ddh > 57) ddh = ddh + 39;
		if (ddl > 57) ddl = ddl + 39;
		pbDest[i*2] = ddh;
		pbDest[i*2+1] = ddl;
	}
	pbDest[nLen*2] = '\0';
}

static char * md5StrTransform(const char *s, char *d)
{
	
		char decrypt[16] = {0};	
		int i;
		MD5_CTX md5;
		MD5Init(&md5);				
		MD5Update(&md5,s,strlen((char *)s));
		MD5Final(&md5,decrypt); 	   
		HexToStr(d,decrypt,16);
		return d;
	
}


static UtilsOps ops = {
		.ByteCrc8 = ByteCrc8,
		.NByteCrc8 = NByteCrc8,
		.YUYVToNV21 = YUYVToNV21,
		.YUYVToNV12 = YUYVToNV12,
		.charToInt  = charToInt,
		.printData = printData,
		.printHex = printHex,
		.GetWeiGendCardId = GetWeiGendCardId,
		.getCpuVer = getCpuVer,
		.getHardWareVer = getHardWareVer,
		.getHardWareFromRK = getHardWareFromRK,
		.md5StrTransform = md5StrTransform,
};
pUtilsOps getUtilsOps(void)
{
	return &ops;
}

