#include <stdio.h>
#include "common/debugLog.h"
#include "YJbackground.h"
#include "updateServer.h"恩
#include "fm1702nl.h"
#include "cardDatabase.h"
#include "common/Utils.h"
#include "common/threadManage.h"
#include "common/timerTaskManage.h"
#include "common/playWav.h"
#include "sample_comm.h"
#include "common/inputEvent.h"


#define DATABASE_PATH  "/usr/work/.cardDB"
#define NEW_DATABASE_PATH "/usr/work/.newcardDB"

#define RESET_LOGIN_TIME 	300
#define GET_CARDID_TIME  	600
#define GET_UPDATE_TIME  	1200
#define OPEN_DOOR_TIME   	5000
#define UP_HEARTBEAT_TIME   595
#define SET_WTD_TIME     	10
static pYJbackgroundOps YjServer;
static pUpdateServerOps updateServer;
static pIcDoorCardOps 	icDoorCardReadServer;
static pCardDataBaseOps cardDateBaseServer;
static pCardDataBaseOps newcardDateBaseServer;



static pThreadOps  wtdThread;


static int YjloginFlag = -1;
static unsigned int timerCount = 0;

/*****
一、登录http后台
二、定时用https get获取房屋卡号,成功获取后就将卡号存储到数据库中
三、定时用https get获取升级信息,发现有新版本发布,则下载升级文件对比MD5值.成功后则升级系统并重启。
四、开启串口读卡监听线程,当用户刷卡时,则用卡号去数据库中查询,成功查询到此卡有注册,并在有效时间范围内,
    则开门，并用http post上传开门记录(15S后关门).同时播放相关语音提示
五、开启线程喂狗,定时喂狗


*****/
static void closeDoor(void *arg)
{
	getUtilsOps()->setDoorSwitch(0);
	LOGI("关门\n");
	
}

static int doorCardRecvFunc(CARD_TYPE type ,unsigned char* cardId,unsigned int cardIdLen)
{
	CardInfo cardPack;
	int ret;
	unsigned long long disabledateTime = 0;
	unsigned long long courentTime = 0;
	char cardStr[12] = {0};


	

	//防止重复刷卡	
	struct timespec  current_time;
	static struct timespec	last_time = {.tv_sec = 0};
	clock_gettime(CLOCK_MONOTONIC, &current_time);
	if ((current_time.tv_sec - last_time.tv_sec)<1)
	{
			return cardIdLen;
	}
	clock_gettime(CLOCK_MONOTONIC, &last_time);



	bzero(&cardPack,sizeof(cardPack));
	getUtilsOps()->GetWeiGendCardId(cardId,cardIdLen,cardStr,sizeof(cardStr));

	
	printf("cardStr:%s\n",cardStr);
	ret = cardDateBaseServer->findCaidId(cardDateBaseServer,cardStr,&cardPack);
	if(ret  == 0)
	{	printf("超时时间:%s\n",cardPack.disabledate);
		disabledateTime = getUtilsOps()->getTimeTick(cardPack.disabledate);
		courentTime = getUtilsOps()->getTimestamp(NULL);
		if(disabledateTime > courentTime)
		{
			printf("门已开\n");
			//开门
			//播放(门已开语音)
			playOpenTheDoor();
			getUtilsOps()->setDoorSwitch(1);
			timerTask(OPEN_DOOR_TIME,closeDoor,NULL);
			//上传IC卡开门记录
			YjServer->upOpendoorRecord(YjServer,cardPack.cardid,cardPack.rid,"S","T");
			
		}else{
			playCardpastdue();
			printf("card:%s disabledateTime: %llu  courentTime: %llu \n",cardPack.cardid,disabledateTime,courentTime);
			printf("卡过期\n");
			YjServer->upOpendoorRecord(YjServer,cardPack.cardid,cardPack.rid,"S","F");
			//播放(卡过期语音)
		}
			
	}else{
		//YjServer->upOpendoorRecord(YjServer,cardStr,0,"S","F");
		printf("卡未登记!\n");
		playCardUnregistered();
		//播放(卡未登记语音)
	}
	return cardIdLen;
}
static int updateCheck(void)
{
	int newVerId;
	int lodVerId;
	char downUrl[256] = {0};
	char md5[128] = {0};
	int ret;
	ret =YjServer->getUpdateRecord(YjServer,&newVerId,downUrl,sizeof(downUrl),md5,sizeof(md5));
	if(ret  != 0)
		return -1;

	
	ret = updateServer->downloadAndUpdate(downUrl,md5,newVerId);
	if(ret != 0)
		return -1;
}
static int getCardIdFromYj(void)
{
	int pageno = 1;
	int pagesize = 400;
	int getsize = 0;
	int totalPage = 0;
	pCardInfo cardPack = malloc(sizeof(CardInfo) * pagesize);
	if(cardPack == NULL)
	{
		return -1;
	}	
	int totalsize = 0;
    while(1)
    {		
			memset(cardPack,0,(sizeof(CardInfo) * pagesize));
			
			totalPage = YjServer->getICcard(YjServer,pageno,cardPack,pagesize, &getsize);
			if(totalPage <= 0){
				break;
			}
			totalsize += getsize;
			printf("totalPage = %d pageno = %d getsize =%d\n",totalPage,pageno,getsize);
			if(getsize > 0)
			{
				if(pageno == 1)
				{	
					newcardDateBaseServer->rebuild(newcardDateBaseServer);  //首次获取把之前的数据清空
				}
				newcardDateBaseServer->addData(newcardDateBaseServer,cardPack,getsize);
			}

			
			pageno ++;
			if( pageno > totalPage)
				break;
				
    }
	newcardDateBaseServer->copyDataBase(newcardDateBaseServer,cardDateBaseServer);
	printf("卡总数 = %d\n",totalsize);
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
		keepWTDalive(SET_WTD_TIME);
		sleep(1);
	}
	closeWTD();
	return NULL;
	
}
static void gpioCallBackFuntion(int code, int value,int state)
{

	switch(code){
		case KEY_A:
			printf("内部开门按钮触发\n");
			getUtilsOps()->setDoorSwitch(1);	
			timerTask(OPEN_DOOR_TIME,closeDoor,NULL);
		break;
		default:
			break;
	}

}

int main(void)
{
	int s32Ret ; 

	
	YjServer = createYJbackgroundServer();
	if(YjServer == NULL)
		goto fail0;

	updateServer =  getUpdateServer();
	icDoorCardReadServer = crateFM1702NLOpsServer("/dev/ttyAMA1",doorCardRecvFunc);
	if(icDoorCardReadServer == NULL)
	{
		LOGE("fail to crateFM1702NLOpsServer");
		goto fail1;
	}
	cardDateBaseServer = createCardDataBaseServer(DATABASE_PATH);
	newcardDateBaseServer = createCardDataBaseServer(NEW_DATABASE_PATH);
	
	wtdThread = pthread_register(wtdThreadFunc,NULL,0,NULL);
	wtdThread->start(wtdThread);
	if(cardDateBaseServer == NULL)
	{
		goto fail1;
	}
	keypadInit(gpioCallBackFuntion);
	playStartingUp();

	for(;;)
	{
		
		if(YjloginFlag != 0 ){
	
			if( (timerCount %RESET_LOGIN_TIME == 0 ) || (timerCount < 10) ) {
				LOGD("正在登陆\n");
				YjloginFlag = YjServer->login(YjServer);
				if(YjloginFlag == 0)
				{
					LOGD("登录成功！\n");
					timerCount = 0;
					
				}
			}
		}

		
		if(YjloginFlag == 0 ){
			
			if( timerCount %GET_CARDID_TIME == 0 ) {
				LOGD("获取卡号信息!\n");
				getCardIdFromYj();
			}
			
			if(timerCount % GET_UPDATE_TIME == 0)
			{
				LOGD("检测升级!\n");
				updateCheck();
				
			}
			if(timerCount % UP_HEARTBEAT_TIME == 0){
				LOGD("upHeartbeatPack!");
				YjServer->upHeartbeatPack(YjServer);
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



















































