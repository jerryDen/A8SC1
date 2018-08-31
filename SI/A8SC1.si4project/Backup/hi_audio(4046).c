/******************************************************************************
  A simple program of Hisilicon HI3518 audio input/output/encoder/decoder implementation.
  Copyright (C), 2010-2012, Hisilicon Tech. Co., Ltd.
 ******************************************************************************
    Modification:  2012-7-3 Created
******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#include <stdint.h>
#include "sample_comm.h"
#include "acodec.h"
#include "common/hiAudio.h"
//#include "systemConfig.h"
//#include "us_cam_audio.h"
//#include "uscam_audio.h"
#include "common/commonHead.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#define SYNC_RLOCK(lock,res) do     \
{								    \
	pthread_rwlock_rdlock(&lock);   \
	res;							\
	pthread_rwlock_unlock(&lock);	\
}while(0)
#define SYNC_WLOCK(lock,res) do     \
{								    \
	pthread_rwlock_wrlock(&lock);   \
	res;							\
	pthread_rwlock_unlock(&lock);	\
}while(0)

static  pthread_rwlock_t audio_rw_lock;




static AUDIO_DEV	AoDev = 0;
static AO_CHN      	AoChn = 0;

static AUDIO_DEV   	AiDev = 0;
static AI_CHN		AiChn = 0;
#define A_CHN_NUMS	2	//HI3518Eè¾“å…¥å’Œè¾“å‡ºå„æœ‰2ä¸ªé€šé“ï¼Œå¯¹åº”å·¦å³å£°é“
#define AUDIO_PTNUMPERFRM	160	//èµ„æ–™è¯´åªæœ‰é€‰160æ‰æ”¯æŒèŠ¯ç‰‡è‡ªå¸¦çš„æ¶ˆå›žå£°åŠŸèƒ½

#define EC_RB_ITEM					15	//²¥·Å»º´æÖ¡Êý
#define EC_IN_BYTES					160	//ECº¯ÊýÒ»´Î´¦ÀíµÄ×Ö½ÚÊý, 8k, 10ms,Ã¿´Î´«Èë 160×Ö½Ú
#define AUDIO_BYTES_PER_FRAME		320	//Ã¿Ö¡³¤¶È, 8k, 20ms,320 bytes
#define AUDIO_SAMPLE_RATE			8000




static AUDIO_BIT_WIDTH_E wavFormat;
static AUDIO_SOUND_MODE_E wavChannels;
static HI_U32 wavChunkSize;
static HI_U32 talkChunkSize;
static void * h_mwec = NULL;


#define SAMPLE_DBG(s32Ret)\
do{\
    printf("s32Ret=%#x,fuc:%s,line:%d\n", s32Ret, __FUNCTION__, __LINE__);\
}while(0)






//è®¾ç½®æ’­æ”¾æ—¶çš„éŸ³é‡
int setPlaybackVolume(int val)
{
	printf("set volume, usr val:%d, chip val %02x\n", val, (100-val));
	/*
	  ç”¨æˆ·è®¾ç½®èŒƒå›´0<-->100ï¼ŒèŠ¯ç‰‡è®¾ç½®èŒƒå›´7F<--->0ï¼Œç”¨æˆ·å€¼ä¸º0æ—¶ï¼Œè®¾ç½®å¯¹åº”èŠ¯ç‰‡å€¼0x7fï¼Œé™éŸ³ï¼Œ
	ç”¨æˆ·å€¼1--100å¯¹åº”åˆ°èŠ¯ç‰‡å€¼99--0ï¼ŒèŠ¯ç‰‡å€¼100--126ä¸¢å¼ƒä¸ç”¨
	*/
	char strbuf[100];
	if(val == 0)
		sprintf(strbuf, "himm 0x20050074 0x7f7fa4a4");	//0x7f7f å·¦å³å£°é“é™éŸ³

	else
		sprintf(strbuf, "himm 0x20050074 0x%02x%02xa4a4", (100 - val), (100 - val));

	//ç›´æŽ¥é€šè¿‡è®¾ç½®å¯„å­˜å™¨ï¼Œè®¾ç½®audio codecçš„DACéŸ³é‡åŠå·¦å³å£°é“æ··åˆ
    system(strbuf);
	return 0;
}







/******************************************************************************
* WAVæ’­æ”¾æ—¶ï¼Œåˆå§‹åŒ–éŸ³é¢‘
******************************************************************************/
HI_S32 wavInitPbPcm( AUDIO_BIT_WIDTH_E format, HI_U32 channels, HI_U32 rate, int volume )
{
	HI_S32 s32Ret= HI_SUCCESS;
	AIO_ATTR_S stAioAttr;
	#if 1
	VB_CONF_S stVbConf;
	memset(&stVbConf,0,sizeof(VB_CONF_S));
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_DBG(s32Ret);
		return HI_FAILURE;
	}
	#endif
	
	AUDIO_SAMPLE_RATE_E enInSampleRate = AUDIO_SAMPLE_RATE_8000;
	stAioAttr.enSamplerate = rate;
	stAioAttr.enBitwidth = format;

	//åŒå£°é“æœ‰é—®é¢˜ï¼Œè¿˜æ²¡æœ‰æžå®šï¼Œæš‚åªèƒ½ä½¿ç”¨å•å£°é“
	if( channels == 1 ) stAioAttr.enSoundmode = AUDIO_SOUND_MODE_MONO;
	else stAioAttr.enSoundmode = AUDIO_SOUND_MODE_STEREO;

	stAioAttr.enWorkmode = AIO_MODE_I2S_MASTER;
	stAioAttr.u32EXFlag = 1;

	//è¿™ä¸ªå¯ä»¥ç†è§£æˆALSAçš„å¯åŠ¨é˜€å€¼ï¼Œç¼“å†²æŒ‡å®šçš„å¤„ç†å‘¨æœŸæ•°åŽæ‰å¯åŠ¨æ’­æ”¾ï¼Œæ‰€ä»¥è®¾å¤§äº†ä¸è¡Œ
	stAioAttr.u32FrmNum = 5;	//MAX_AUDIO_FRAME_NUM;

	//æµ·æ€çš„å¸§ï¼Œå¯ç†è§£æˆALSAçš„å¤„ç†å‘¨æœŸï¼Œæœ‰å‡ ä¸ªå¯é€‰çš„å›ºå®šå€¼
	stAioAttr.u32PtNumPerFrm = AUDIO_PTNUMPERFRM;

	stAioAttr.u32ChnCnt = A_CHN_NUMS;
	stAioAttr.u32ClkSel = 1;

	s32Ret = SAMPLE_COMM_AUDIO_CfgAcodec(&stAioAttr);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_DBG(s32Ret);
		return HI_FAILURE;
	}
	s32Ret = SAMPLE_COMM_AUDIO_StartAo(AoDev, A_CHN_NUMS, &stAioAttr,enInSampleRate,HI_FALSE,NULL,HI_FALSE);
	//s32Ret = SAMPLE_COMM_AUDIO_StartAo(AoDev, AoChn, &stAioAttr, NULL);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_DBG(s32Ret);
		return HI_FAILURE;
	}
	#if 1
	setPlaybackVolume(volume);

	wavFormat	= format;
	wavChannels = stAioAttr.enSoundmode;
	switch( format ){
		case AUDIO_BIT_WIDTH_8:  wavChunkSize = AUDIO_PTNUMPERFRM * channels;	 break;
		case AUDIO_BIT_WIDTH_16: wavChunkSize = AUDIO_PTNUMPERFRM * channels * 2; break;
		//case AUDIO_BIT_WIDTH_32: wavChunkSize = AUDIO_PTNUMPERFRM * channels * 4; break;
		default: wavChunkSize = AUDIO_PTNUMPERFRM * channels * 4; break;
	}
	printf("(*)wavInitPbPcm success! format %d, channels %d, rate %d\n", format, channels, rate);
	#endif
	return HI_SUCCESS;
}


HI_VOID wavExitPbPcm(HI_VOID)
{
	SAMPLE_COMM_AUDIO_StopAo(AoDev, A_CHN_NUMS, HI_FALSE,HI_FALSE);
}


HI_U32 GetWavChunkBytes(HI_VOID)
{
	return wavChunkSize;
}


HI_S32 wavWritePcm( HI_VOID *buf, HI_U32 bytes )
{
	HI_S32 s32Ret= HI_SUCCESS;
	AUDIO_FRAME_S stFrame;

	stFrame.enBitwidth  = wavFormat;
	stFrame.enSoundmode = wavChannels;
	stFrame.u32Len = wavChunkSize;

	HI_S32 i = (HI_S32)bytes;
	while( i > 0 ){
		stFrame.pVirAddr[0] = buf;
//		stFrame.pVirAddr[1] = buf;
		s32Ret = HI_MPI_AO_SendFrame(AoDev, AoChn, &stFrame, HI_TRUE);
		if( s32Ret != HI_SUCCESS ){
			SAMPLE_DBG(s32Ret);
	        return HI_FAILURE;
		}
		i -= wavChunkSize;
		buf = (char*)buf + wavChunkSize;
	}
	return HI_SUCCESS;
}
HI_S32 us_wavWritePcm(HI_VOID *buf, HI_U32 bytes )
{
	HI_S32 i = (HI_S32)bytes;
	HI_S32 s32Ret= HI_SUCCESS;

	while(i >= wavChunkSize)
	{
		
		//s32Ret = UsCamAudioPlay(buf,i);
		if(s32Ret != HI_SUCCESS)
		{
			SAMPLE_DBG(s32Ret);
	        return HI_FAILURE;
		}
		i -= wavChunkSize;
		buf = (char*)buf + wavChunkSize;	
	}
	return HI_SUCCESS;
}




/******************************************************************************
*
******************************************************************************/


























#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
