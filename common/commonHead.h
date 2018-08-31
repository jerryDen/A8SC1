#ifndef  COMMON_HEAD_H__
#define  COMMON_HEAD_H__


#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/select.h>
#include <stdlib.h>
#include "DebugLog.h"



#define DATAMAX 1500

#define  PRINT_LINE {printf("[file]:%s [func]:%s [line]:%d\n",__FILE__,__func__,__LINE__);}


#define FREE_DATAPACKET(dataPack) do    \
{										\
	if((dataPack).data)					\
		free((dataPack).data);          \
	(dataPack).data = NULL;				\
}while(0) 
#define PRINT_ARRAY(array,arrayLen) do    \
{										  \
		int i;							  \
		for(i = 0; i<(arrayLen);i++){	  \
			printf("0x%x ", (array)[i]);  \
		}								  \
		printf("\n");					  \
}while(0)

#define CHECK_RET(ret,log,cmd) do 	\
{									\
		if(ret)						\
		{							\
			printf("[%s:%d]%s\n",__func__,__LINE__,log);		\
			cmd;					\
		}							\
									\
}while(0)
typedef unsigned char uchar;
typedef unsigned int  uint;
typedef  struct DataPacket_Static DataPacket_Static,*pDataPacket_Static;
typedef  struct DataPacket_Malloc DataPacket_Malloc,*pDataPacket_Malloc;





struct DataPacket_Static{
		uint  dataLen;
 		uchar data[DATAMAX];
		
};

struct DataPacket_Malloc{
		uint  dataLen;
		uchar *data;
		
};


	                   
#endif





















