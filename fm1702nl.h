#ifndef  __WB_ICDOORCARD_H
#define  __WB_ICDOORCARD_H
#include <stdint.h>
typedef union {
 		char buf[sizeof(uint32_t)];
 		uint32_t id;
}intChar_union;



typedef enum{
	IC_CARD=0,
	CPU_CARD,
	ID_CARD,
}CARD_TYPE;

typedef int (*DoorCardRecvFunc)(CARD_TYPE,unsigned char*,unsigned int);



typedef struct{


}IcDoorCardOps,*pIcDoorCardOps;

pIcDoorCardOps crateFM1702NLOpsServer(const unsigned char *devPath,DoorCardRecvFunc recvFunc);
void  destroyFM1702NLOpsServer(pIcDoorCardOps* server);





#endif
