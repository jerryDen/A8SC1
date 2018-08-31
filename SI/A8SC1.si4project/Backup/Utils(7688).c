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
#include <time.h>


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
#include "cJSON.h"
#include "Utils.h"
#include "DebugLog.h"

#define DEBUGLEVEL 7

#define ETHTOOL_GLINK  0x0000000a /* Get link status (ethtool_value) */

#define APP_VER_FILE   "/usr/work/.app_version"

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
 static int GetWeiGendCardId(unsigned char* rawData,int rawDatalen,char* validData,int validLen)
{
	int rawDawInt[3] = {0};
	int hightInt = 0;
	int i ;
	if(rawDatalen< 3||validLen <8)
	{
		return -1;
	}
	for(i= 0;i < 3;i++)
	{
		if(rawData[i] < 0)
			rawDawInt[i] = 256 + (int)rawData[i];
		else
			rawDawInt[i] = rawData[i];
	}
	hightInt  = (rawDawInt[1]<<8) | (rawDawInt[0]);

	return sprintf(validData,"%03u%05u",rawDawInt[2],hightInt);
}

static int GetWeiGendCardIdold(const char *hexIn,int len, int *id)
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
static int getAPPver(char *ver)
{
	 char appver[12] = "1000";
	FILE*stream;


    stream = fopen(APP_VER_FILE,"r");


     fgets(appver,sizeof(appver),stream);
     printf(" %s",appver);
     fclose(stream);
	 if(ver != NULL)
	 	strcpy(ver,appver);
	 return atoi(appver);
}
static int setAPPver(int ver)
{

	 FILE*stream;
	 int ret;
	 char verStr[12]  = {0};
	 sprintf(verStr,"%d",ver);

     stream = fopen(APP_VER_FILE,"w+");
	 if(stream == NULL)
	 		return -1;
	
     ret  = fwrite(verStr,1,strlen(verStr),stream);
	 if(ret <= 0)
	 	return -1;
     fclose(stream);
	 return 0;
	
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
 


static char * md5FileTransform(const char *file_path, char *md5_str)
{
	#define READ_DATA_SIZE	1024
	#define MD5_SIZE		16
	#define MD5_STR_LEN		(MD5_SIZE * 2)
	int i;
	int fd;
	int ret;
	unsigned char data[READ_DATA_SIZE];
	unsigned char md5_value[MD5_SIZE];
	MD5_CTX md5;
 
	fd = open(file_path, O_RDONLY);
	if (-1 == fd)
	{
		perror("open");
		return NULL;
	}
 
	// init md5
	MD5Init(&md5);
	while (1)
	{
		ret = read(fd, data, READ_DATA_SIZE);
		if (-1 == ret)
		{
			perror("read");
			return NULL;
		}
 
		MD5Update(&md5, data, ret);
 
		if (0 == ret || ret < READ_DATA_SIZE)
		{
			break;
		}
	}
 
	close(fd);
 
	MD5Final(&md5, md5_value);
 	HexToStr(md5_str,md5_value,sizeof(md5_value));
	/*
	for(i = 0; i < MD5_SIZE; i++)
	{
		snprintf(md5_str + i*2, 2+1, "%02x", md5_value[i]);
	}
	md5_str[MD5_STR_LEN] = '\0'; // add end
	*/
 
	return md5_str;
}

static char * md5FileTransformold(const char *fileName, char *d)
{
	unsigned long filesize = -1; 
	char decrypt[16] = {0};	
	char *fileBuf;
	int readn = 0;
	int ret;
    FILE *fp;  
	long curpos;
    fp = fopen(fileName, "rb");  
    if(fp == NULL)  
        return NULL;  

    fseek(fp, 0L, SEEK_END);  
    filesize = ftell(fp); 
	
	printf("filesize:%d\n",filesize);
	ret = fseek(fp,0,SEEK_SET);
	
	fileBuf = malloc(filesize);
	if(fileBuf == NULL)
		goto fail0;
	
	while(readn < filesize){
		ret = fread(fileBuf,1,filesize,fp);
		if(ret == 0)
		{
			break;
		}
		readn += ret;
	}
	if(readn != filesize)
	{
		goto fail1;
	}
	
	MD5_CTX md5;
	MD5Init(&md5);				
	MD5Update(&md5,fileBuf,filesize);
	MD5Final(&md5,decrypt); 	   
	HexToStr(d,decrypt,sizeof(decrypt));
	free(fileBuf);
	fclose(fp);
	return d;
	fail1:
		free(fileBuf);
	fail0:
		fclose(fp);
		return NULL;
}
static int getStrFormCjson(const char *cjsonStr,const char *keyStr,char *buf,int len)
{
	 cJSON *jsonfind;
	 int valuestrLen;
	 cJSON *jsonroot = cJSON_Parse(cjsonStr);  
	 if(jsonroot == NULL)
	 	return -1; 
    jsonfind  = cJSON_GetObjectItem(jsonroot, keyStr); 
	if(jsonfind == NULL)
		return -1;
	valuestrLen = strlen(jsonfind->valuestring);
	if( valuestrLen< len)
	{
		memcpy(buf,jsonfind->valuestring,valuestrLen);
		buf[valuestrLen] = 0;
		return valuestrLen+1;
	}else {

		memcpy(buf,jsonfind->valuestring,len-1);
		buf[len] = 0;
		return len;
	}

	return -1;
}
static int  getIntFormCjson(const char *cjsonStr,const char *keyStr,int *value)
{
	 cJSON *jsonroot = cJSON_Parse(cjsonStr); 
	 cJSON *jsonfind;

	 if(jsonroot == NULL)
	 	return -1; 
     jsonfind  = cJSON_GetObjectItem(jsonroot, keyStr);
	 if(jsonfind == NULL ){
	 	return -1; 
	 }
	 *value = jsonfind->valueint;

	 return 0;
}
static int getChildDoubleFormCjson(const char *cjsonStr,const char *childStr,const char *keyStr,double *value)
{
	cJSON *jsonroot = cJSON_Parse(cjsonStr);  
	cJSON *child;
	cJSON *find;
	if(jsonroot == NULL)
	 	return -1; 
	child  = cJSON_GetObjectItem(jsonroot, childStr); 
	if(child == NULL)
		return -1;
	
	find = cJSON_GetObjectItem(child,keyStr);
	if(find == NULL)
	{
		return -1;
	}
	*value = find->valuedouble;
	return 0;
}
static int getChildIntFormCjson(const char *cjsonStr,const char *childStr,const char *keyStr,int *value)
{
	cJSON *jsonroot = cJSON_Parse(cjsonStr);  
	cJSON *child;
	cJSON *find;
	if(jsonroot == NULL)
	 	return -1; 
	child  = cJSON_GetObjectItem(jsonroot, childStr); 
	if(child == NULL)
		return -1;
	find = cJSON_GetObjectItem(child,keyStr);
	if(find == NULL)
	{
		return -1;
	}
	*value = find->valueint;
	return 0;
}
static int getChildStrFormCjson(const char *cjsonStr,const char *childStr,const char *keyStr,char *buf,int len)
{
	cJSON *jsonroot = cJSON_Parse(cjsonStr);  
	cJSON *child;
	cJSON *find;
	int valuestrLen;
	if(jsonroot == NULL)
	 	return -1; 
	child  = cJSON_GetObjectItem(jsonroot, childStr); 
	if(child == NULL)
		return -1;
	find = cJSON_GetObjectItem(child,keyStr);
	if(find == NULL)
	{
		return -1;
	}
	valuestrLen = strlen(find->valuestring);
	if( valuestrLen< len)
	{
		memcpy(buf,find->valuestring,valuestrLen);
		buf[valuestrLen] = 0;
		return valuestrLen+1;
	}else {

		memcpy(buf,find->valuestring,len-1);
		buf[len] = 0;
		return len;
	}
	return -1;
}


static int  getDoubleFormCjson(const char *cjsonStr,const char *keyStr ,double *value)
{
	 cJSON *jsonfind;
	 cJSON *jsonroot = cJSON_Parse(cjsonStr);  
	 if(jsonroot == NULL)
	 	return -1; 
    jsonfind = cJSON_GetObjectItem(jsonroot, keyStr); 
	 if(jsonfind == NULL)
	 	return -1;
	 *value = jsonfind->valuedouble;
	 return 0;
}
static int getMacAddress(char *addrBuf)
{	
	   struct ifreq ifreq;
	   int sock;
	   if(addrBuf == NULL)
	   		return -1;
	   if ((sock = socket (AF_INET, SOCK_STREAM, 0)) < 0)
	   {
		   perror ("socket");
		   return -1;
	   }
	   strcpy (ifreq.ifr_name, "eth0");    //Currently, only get eth0
	   if (ioctl (sock, SIOCGIFHWADDR, &ifreq) < 0)
	   {
		   perror ("ioctl");
		   return -1;
	   }
	   close(sock);
	   return sprintf (addrBuf, "%02x:%02x:%02x:%02x:%02x:%02x", (unsigned char) ifreq.ifr_hwaddr.sa_data[0], (unsigned char) ifreq.ifr_hwaddr.sa_data[1],
	   				(unsigned char) ifreq.ifr_hwaddr.sa_data[2], (unsigned char) ifreq.ifr_hwaddr.sa_data[3],
	   				(unsigned char) ifreq.ifr_hwaddr.sa_data[4], (unsigned char) ifreq.ifr_hwaddr.sa_data[5]);
	
}
static unsigned long long  getTimestamp(char * timestamp)
{
	 unsigned long long  TimestampInt = 0;
	 struct timeval tv;
     gettimeofday(&tv,NULL);
	 TimestampInt = (tv.tv_sec*1000LL + tv.tv_usec/1000);	
	 if(timestamp != NULL){
	 	sprintf(timestamp,"%llu",TimestampInt);
	 }
	 return TimestampInt;
}
static void setDoorSwitch(int action)
{

	//GPIO6_7;
	//muxctrl_reg55  0x200f00dc  -- > 0
	//GPIO6  0x201A_0000
	
	//GPIO_DATA 0x200;  0X1;
	//GPIO_DIR  0X400   0X80
	
	system("himm 0x200f00dc   0x0");
	system("himm 0x201A0400   0x80");
	if(action)
		system("himm 0x201A0200   0x80");
	else
		system("himm 0x201A0200   0x0");

	
}

static unsigned long long  getTimeTick(const char *str_time)
{
		struct tm stm;
		int iY, iM, iD, iH, iMin, iS; 
		memset(&stm,0,sizeof(stm));
	 
		iY = atoi(str_time);
		iM = atoi(str_time+5);
		iD = atoi(str_time+8);
		iH = atoi(str_time+11);
		iMin = atoi(str_time+14);
		iS = atoi(str_time+17);
	 
		stm.tm_year=iY-1900;
		stm.tm_mon=iM-1;
		stm.tm_mday=iD;
		stm.tm_hour=iH;
		stm.tm_min=iMin;
		stm.tm_sec=iS;
		/*printf("%d-%0d-%0d %0d:%0d:%0d\n", iY, iM, iD, iH, iMin, iS);*/
		return (unsigned long long )mktime(&stm);
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
		.md5FileTransform = md5FileTransform,
		.getStrFormCjson = getStrFormCjson,
		.getIntFormCjson = getIntFormCjson,
		.getChildStrFormCjson = getChildStrFormCjson,
		.getChildDoubleFormCjson = getChildDoubleFormCjson,
		.getChildIntFormCjson = getChildIntFormCjson,
		.getDoubleFormCjson = getDoubleFormCjson,
		.getMacAddress = getMacAddress,
		.getTimestamp = getTimestamp,
		.getTimeTick = getTimeTick,
		.getAPPver = getAPPver,
		.setAPPver = setAPPver,
		.setDoorSwitch = setDoorSwitch,
};	
pUtilsOps getUtilsOps(void)
{
	return &ops;
}

