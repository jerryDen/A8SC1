
#ifndef  CARD_DATABASE__H__
#define  CARD_DATABASE__H__


typedef struct CardInfo{
	char disabledate[36];
	int untitd;
	char state[3];
	int cellid;
	int rid;
	int blockid;
	char type[3];
	int districtid;  
	char cardid[16];
	
}CardInfo,*pCardInfo;





typedef struct CardDataBaseOps{

	void (*rebuild)(struct CardDataBaseOps * ops);
	int (*addData)(struct CardDataBaseOps* ops,pCardInfo cardArray,int cardNum);
	int (*findCaidId)(struct CardDataBaseOps* ops,char cardid[16]);
	

}CardDataBaseOps,*pCardDataBaseOps;


pCardDataBaseOps createCardDataBaseServer(void);

void destroyCardDataBaseServer(pCardDataBaseOps server);


#endif
























