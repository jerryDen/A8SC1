#include <stdio.h>
#include "common/debugLog.h"
#include "YJbackground.h"
#include "updateServer.h"
#include "fm1702nl.h"
#include "cardDatabase.h"
#include "common/Utils.h"
#include "common/threadManage.h"



#define GET_CARDID_TIME  6
#define GET_UPDATE_TIME  6
#define SET_WTD_TIME     1
static pYJbackgroundOps YjServer;
static pUpdateServerOps updateServer;
static pIcDoorCardOps 	icDoorCardReadServer;
static pCardDataBaseOps cardDateBaseServer;
static pThreadOps  wtdThread;
static pTimerOps closeDoorTimer;


static int YjloginFlag = -1;
static unsigned int timerCount = 0;


static int doorCardRecvFunc(CARD_TYPE type ,unsigned char* cardId,unsigned int cardIdLen)
{
	CardInfo cardPack;
	int ret;
	double disabledateTime;
	double courentTime;
	union {
		char buf[sizeof(unsigned int)];
		unsigned int id;
	} cardNum;
	bzero(&cardNum, sizeof(cardNum));
	bzero(&cardPack,sizeof(cardPack));
	getUtilsOps()->GetWeiGendCardId(cardId, cardIdLen, &(cardNum.id));
	ret = cardDateBaseServer->findCaidId(cardDateBaseServer,cardNum.buf,&cardPack);
	if(ret  == 0)
	{
		disabledateTime = getUtilsOps()->getTimeTick(cardPack.disabledate);
		courentTime = getUtilsOps()->getTimestamp(NULL);
		if(disabledateTime > courentTime)
		{

			//开门
			//播放(门已开语音)
			
		}else{
			//播放(卡过期语音)
		}
			
	}else{

		//播放(卡未登记语音)
	}
}
static int updateCheck(void)
{
	int newVerId;
	int lodVerId;
	char downUrl[256] = {0};
	char md5[48] = {0};
	int ret;
	ret =YjServer->getUpdateRecord(YjServer,&newVerId,downUrl,sizeof(downUrl),md5,sizeof(md5));
	if(ret  != 0)
		return -1;

	lodVerId = getUtilsOps()->getAPPver(NULL);
	if(newVerId > lodVerId ){
	ret = updateServer->downloadAndCheck(downUrl,md5);
	if(ret != 0)
		return -1;
		updateServer->update();
	}	
}
static int getCardIdFromYj(void)
{
	int pageno = 1;
	int pagesize = 400;
	int getsize = 0;
	int totalPage = 0;
	int remainPage = 1;
	pCardInfo cardPack = malloc(sizeof(CardInfo) * pagesize);
	if(cardPack == NULL)
	{
		return -1;
	}	
    while(remainPage >0)
    {
			memset(cardPack,0,(sizeof(CardInfo) * pagesize));
			totalPage = YjServer->getICcard(YjServer,pageno,cardPack,pagesize, &getsize);
			
			if(getsize > 0)
			{
				printf("getCardIdFromYj succeed\n");
				if(pageno == 1)
				{	
					cardDateBaseServer->rebuild(cardDateBaseServer);  //首次获取把之前的数据清空
					remainPage = totalPage - 1; 
				}
				cardDateBaseServer->addData(cardDateBaseServer,cardPack,getsize);
			}
			remainPage --;
			pageno ++;
    }
	free(cardPack);
	return 0;

}
static void * wtdThreadFunc(void* arg)
{
	int ret =  openWTD();
	if(ret  < 0)
		return NULL;

	for(;;)
	{
		keepWTDalive(5);
		sleep(1);
	}


	closeWTD();
	return NULL;
	
}
int main(void)
{

	YjServer = createYJbackgroundServer();
	if(YjServer == NULL)
		goto fail0;

	updateServer =  getUpdateServer();
	icDoorCardReadServer = crateFM1702NLOpsServer("dev/ttyAMA1",doorCardRecvFunc);
	cardDateBaseServer = createCardDataBaseServer();
	
	closeDoorTimer = createTimerTaskServer( int startTime, int loopTime,int loopCount,int startThread,pThreadFunc taskFunc,void *arg,int argLen);

	wtdThread = pthread_register(wtdThreadFunc,NULL,0,NULL);
	wtdThread->start(wtdThread);
	if(cardDateBaseServer == NULL)
	{
		goto fail1;
	}
	for(;;)
	{
		if(YjloginFlag != 0 )
			YjloginFlag = YjServer->login(YjServer);
		if(YjloginFlag == 0 ){
			if( timerCount %GET_CARDID_TIME == 0)
				getCardIdFromYj();
			
			if(timerCount % GET_UPDATE_TIME == 0)
			{
				updateCheck();
			}
		}
	
		timerCount ++;
		sleep(1);
		
	}



	return 0;
fail1:
		
fail0:
		return -1;
}



















































