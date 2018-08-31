/******************************************************************************
  A simple program of Hisilicon mpp audio input/output/encoder/decoder implementation.
  Copyright (C), 2010-2021, Hisilicon Tech. Co., Ltd.
 ******************************************************************************
    Modification:  2013-7 Created
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

#include "sample_comm.h"
#include "acodec.h"
#include "tlv320aic31.h"
#if 0

#define SOUNDMODE AUDIO_SOUND_MODE_MONO

#else
	
#define SOUNDMODE AUDIO_SOUND_MODE_STEREO

#endif
#define CHNCNT 2

static PAYLOAD_TYPE_E gs_enPayloadType = PT_LPCM;

//static HI_BOOL gs_bMicIn = HI_FALSE;

static HI_BOOL gs_bAioReSample  = HI_FALSE;
static HI_BOOL gs_bUserGetMode  = HI_FALSE;
static HI_BOOL gs_bAoVolumeCtrl = HI_TRUE;
static AUDIO_SAMPLE_RATE_E enInSampleRate  = AUDIO_SAMPLE_RATE_BUTT;
static AUDIO_SAMPLE_RATE_E enOutSampleRate = AUDIO_SAMPLE_RATE_BUTT;
static HI_U32 u32AencPtNumPerFrm = 0;
/* 0: close, 1: open*/
static HI_U32 u32AiVqeType = 1;  
/* 0: close, 1: open*/
static HI_U32 u32AoVqeType = 1;  

static int setMicGain(int gain);
#define SAMPLE_DBG(s32Ret)\
do{\
    printf("s32Ret=%#x,fuc:%s,line:%d\n", s32Ret, __FUNCTION__, __LINE__);\
}while(0)

/******************************************************************************
* function : PT Number to String
******************************************************************************/
static char* SAMPLE_AUDIO_Pt2Str(PAYLOAD_TYPE_E enType)
{
    if (PT_G711A == enType)  
    {
        return "g711a";
    }
    else if (PT_G711U == enType)  
    {
        return "g711u";
    }
    else if (PT_ADPCMA == enType)  
    {
        return "adpcm";
    }
    else if (PT_G726 == enType) 
    {
        return "g726";
    }
    else if (PT_LPCM == enType)  
    {
        return "pcm";
    }
    else 
    {
        return "data";
    }
}

/******************************************************************************
* function : Open Aenc File
******************************************************************************/
static FILE * SAMPLE_AUDIO_OpenAencFile(AENC_CHN AeChn, PAYLOAD_TYPE_E enType)
{
    FILE* pfd;
    HI_CHAR aszFileName[FILE_NAME_LEN];

    /* create file for save stream*/
    snprintf(aszFileName, FILE_NAME_LEN, "audio_chn%d.%s", AeChn, SAMPLE_AUDIO_Pt2Str(enType));
    pfd = fopen(aszFileName, "w+");
    if (NULL == pfd)
    {
        printf("%s: open file %s failed\n", __FUNCTION__, aszFileName);
        return NULL;
    }
    printf("open stream file:\"%s\" for aenc ok\n", aszFileName);
    return pfd;
}

/******************************************************************************
* function : Open Adec File
******************************************************************************/
static FILE *SAMPLE_AUDIO_OpenAdecFile(ADEC_CHN AdChn, PAYLOAD_TYPE_E enType)
{
    FILE* pfd;
    HI_CHAR aszFileName[FILE_NAME_LEN];

    /* create file for save stream*/
    snprintf(aszFileName, FILE_NAME_LEN ,"audio_chn%d.%s", AdChn, SAMPLE_AUDIO_Pt2Str(enType));
    pfd = fopen(aszFileName, "rb");
    if (NULL == pfd)
    {
        printf("%s: open file %s failed\n", __FUNCTION__, aszFileName);
        return NULL;
    }
    printf("open stream file:\"%s\" for adec ok\n", aszFileName);
    return pfd;
}


/******************************************************************************
* function : file -> Adec -> Ao
******************************************************************************/
HI_S32 SAMPLE_AUDIO_AdecAo(HI_VOID)
{
    HI_S32      s32Ret;
    AUDIO_DEV   AoDev = SAMPLE_AUDIO_AO_DEV;
    AO_CHN      AoChn   = 0;
    AO_CHN      AoChn1 = 2;

    ADEC_CHN    AdChn = 0;
    HI_S32      s32AoChnCnt;
    FILE*        pfd = NULL;
    AIO_ATTR_S stAioAttr;







#ifdef HI_ACODEC_TYPE_TLV320AIC31
    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = SOUNDMODE;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = SAMPLE_AUDIO_PTNUMPERFRM;
    stAioAttr.u32ChnCnt      = CHNCNT;
    stAioAttr.u32ClkSel      = 1;
#else
    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = SOUNDMODE;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = SAMPLE_AUDIO_PTNUMPERFRM;
    stAioAttr.u32ChnCnt      = CHNCNT;
    stAioAttr.u32ClkSel      = 0;
#endif


    gs_bAioReSample = HI_FALSE;
    enInSampleRate  = AUDIO_SAMPLE_RATE_BUTT;
    enOutSampleRate = AUDIO_SAMPLE_RATE_BUTT;
	
    s32Ret = SAMPLE_COMM_AUDIO_CfgAcodec(&stAioAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }
    
    s32Ret = SAMPLE_COMM_AUDIO_StartAdec(AdChn, gs_enPayloadType);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }

	s32AoChnCnt = stAioAttr.u32ChnCnt;
    s32Ret = SAMPLE_COMM_AUDIO_StartAo(AoDev, s32AoChnCnt, &stAioAttr, enInSampleRate, gs_bAioReSample, NULL, 0);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }
	//add by mrzhang
	#if 1
	AUDIO_TRACK_MODE_E enTrackMode = AUDIO_TRACK_BOTH_LEFT;

	s32Ret = HI_MPI_AO_SetTrackMode(AoDev, enTrackMode);
	if (HI_SUCCESS != s32Ret)
	{
	printf("Ao set track mode failure!\n");
	return s32Ret;
	}
	printf("mrzhang audio out test!!!\n");
	#endif 
    s32Ret = SAMPLE_COMM_AUDIO_AoBindAdec(AoDev, AoChn, AdChn);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }
	
    pfd = SAMPLE_AUDIO_OpenAdecFile(AdChn, gs_enPayloadType);
    if (!pfd)
    {
        SAMPLE_DBG(HI_FAILURE);
        return HI_FAILURE;
    }
	printf("open file successful*************\n");
    s32Ret = SAMPLE_COMM_AUDIO_CreatTrdFileAdec(AdChn, pfd);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }
    
    printf("bind adec:%d to ao(%d,%d) ok \n", AdChn, AoDev, AoChn);

    printf("\nplease press twice ENTER to exit this sample\n");
    getchar();
    getchar();

    s32Ret = SAMPLE_COMM_AUDIO_DestoryTrdFileAdec(AdChn);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }
    
    s32Ret = SAMPLE_COMM_AUDIO_StopAdec(AdChn);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }
    
    s32Ret = SAMPLE_COMM_AUDIO_StopAo(AoDev, s32AoChnCnt, gs_bAioReSample, HI_FALSE);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }
    
    s32Ret = SAMPLE_COMM_AUDIO_AoUnbindAdec(AoDev, AoChn, AdChn);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }
    
    return HI_SUCCESS;
}

/******************************************************************************
* function : Ai -> Aenc -> file
*                       -> Adec -> Ao
******************************************************************************/


HI_S32 SAMPLE_AUDIO_AiAenc(HI_VOID)
{
    HI_S32 i, s32Ret;
    AUDIO_DEV   AiDev = SAMPLE_AUDIO_AI_DEV;
    AI_CHN      AiChn;
    AUDIO_DEV   AoDev = SAMPLE_AUDIO_AO_DEV;
    AO_CHN      AoChn = 0;
    ADEC_CHN    AdChn = 0;
    HI_S32      s32AiChnCnt;
	HI_S32      s32AoChnCnt;
    HI_S32      s32AencChnCnt;
    AENC_CHN    AeChn;
    HI_BOOL     bSendAdec = HI_TRUE;
    FILE        *pfd = NULL;
    AIO_ATTR_S stAioAttr;
	
	void * pAiVqeAttr  =NULL;

#ifdef HI_ACODEC_TYPE_TLV320AIC31
    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = SAMPLE_AUDIO_PTNUMPERFRM;
    stAioAttr.u32ChnCnt      = CHNCNT;
    stAioAttr.u32ClkSel      = 1;
#else
    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = SAMPLE_AUDIO_PTNUMPERFRM;
    stAioAttr.u32ChnCnt      = CHNCNT;
    stAioAttr.u32ClkSel      = 0;
#endif
    gs_bAioReSample = HI_FALSE;
    enInSampleRate  = AUDIO_SAMPLE_RATE_BUTT;
    enOutSampleRate = AUDIO_SAMPLE_RATE_BUTT;
	u32AencPtNumPerFrm = stAioAttr.u32PtNumPerFrm;

	AI_VQE_CONFIG_S *pstAiVqeAttr;
	AI_VQE_CONFIG_S stAiVqeAttr;
	
	stAiVqeAttr.s32WorkSampleRate	 = AUDIO_SAMPLE_RATE_8000;
	stAiVqeAttr.s32FrameSample		 = SAMPLE_AUDIO_PTNUMPERFRM;
	stAiVqeAttr.enWorkstate 		 = VQE_WORKSTATE_COMMON;
	
	//高通滤波HPF（ high-pass filte） 主要负责去除低频噪声。
	stAiVqeAttr.bHpfOpen			 = HI_FALSE;//HI_TRUE;
	stAiVqeAttr.stHpfCfg.bUsrMode	 = HI_FALSE;
	//stAiVqeAttr.stHpfCfg.enHpfFreq	 = AUDIO_HPF_FREQ_150;

	//回声抵消AEC	（ Acoustic Echo Cancellation） 
	stAiVqeAttr.bAecOpen			 = HI_FALSE;//HI_TRUE;//回声抵消功能是否使能标志。
	stAiVqeAttr.stAecCfg.bUsrMode	 = HI_FALSE;//0: auto mode
	stAiVqeAttr.stAecCfg.s8CngMode	 = 0;//是否开启舒适噪声模式cozy noisy mode: 0 close,

	//语音降噪ANR（ Audio Noise Reduction）更适用于 NVR 和 IPC场景
	stAiVqeAttr.bAnrOpen			 = HI_FALSE;
	stAiVqeAttr.stAnrCfg.bUsrMode	 = HI_FALSE;

	//自动增益AGC 放大输入源的声音，以保证音源过小时，经过算法处理后的声音依然很大。 
	stAiVqeAttr.bAgcOpen			 = HI_TRUE;//自动增益控制功能是否使能标志
	stAiVqeAttr.stAgcCfg.bUsrMode	 = HI_FALSE;

	//EQ均衡器处理EQ
	stAiVqeAttr.bEqOpen 			 = HI_FALSE;

	//录音噪声消除功能是否使能标志。RNR 适用于运动 DV 场景。
	stAiVqeAttr.bRnrOpen			 = HI_FALSE;

	//该通道是否启用了高动态范围功能
	stAiVqeAttr.bHdrOpen			 = HI_FALSE;
	
	pstAiVqeAttr = &stAiVqeAttr;












	
		
    /********************************************
      step 1: config audio codec
    ********************************************/
	
    s32Ret = SAMPLE_COMM_AUDIO_CfgAcodec(&stAioAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }

    /********************************************
      step 2: start Ai
    ********************************************/
    s32AiChnCnt = 1;//stAioAttr.u32ChnCnt; 
    s32Ret = SAMPLE_COMM_AUDIO_StartAi(AiDev, s32AiChnCnt, &stAioAttr, enOutSampleRate, gs_bAioReSample, pstAiVqeAttr, 1);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }
	//set mic gain
	int fdAcodec = open( "/dev/acodec", O_RDWR );
	ACODEC_VOL_CTRL input_vol_ctrl;
	int ret;
	input_vol_ctrl.vol_ctrl_mute = 0x0;//1:mute, 0:unmute*/
	input_vol_ctrl.vol_ctrl = 0x01;//0x20;//0x00~0x7e, 0x7F:mute*/
	printf( "Aenc: intputMute = %02x inputVol = %02x \n", input_vol_ctrl.vol_ctrl_mute, input_vol_ctrl.vol_ctrl );
	if( ioctl( fdAcodec, ACODEC_SET_ADCL_VOL, &input_vol_ctrl ) )
	{
		printf( "ioctl err!\n" );
	}
	close( fdAcodec );

	//打开声音质量增强的自动增益，其它设置声音的都无效
	//只能用这个接口函数调节，范围[-20,10]
	ret = HI_MPI_AI_SetVqeVolume(0,0,-12);



	#if 0
	
		AUDIO_TRACK_MODE_E temp = AUDIO_TRACK_BOTH_LEFT;
		temp = 0;
		s32Ret = HI_MPI_AI_SetTrackMode(AiDev, temp);
		if (HI_SUCCESS != s32Ret)
		{
		printf("Ai set track mode failure!\n");
		return s32Ret;
		}

		
	s32Ret = HI_MPI_AI_GetTrackMode(AiDev, &temp);
	;
	if (HI_SUCCESS != s32Ret)
	{
		printf("Ai get track mode failure! AiDev: %d, s32Ret: 0x%x.\n", AiDev,s32Ret);
		return s32Ret;
	}
	printf("#################mrzhang###track mode:%d\n",temp);
#endif
    /********************************************
      step 3: start Aenc
    ********************************************/
    s32AencChnCnt = 1;
    s32Ret = SAMPLE_COMM_AUDIO_StartAenc(s32AencChnCnt, u32AencPtNumPerFrm, gs_enPayloadType);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }

    /********************************************
      step 4: Aenc bind Ai Chn
    ********************************************/
    for (i=0; i<s32AencChnCnt; i++)
    {
        AeChn = i;
        AiChn = i;

        if (HI_TRUE == gs_bUserGetMode)
        {
            s32Ret = SAMPLE_COMM_AUDIO_CreatTrdAiAenc(AiDev, AiChn, AeChn);
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_DBG(s32Ret);
                return HI_FAILURE;
            }
        }
        else
        {        
            s32Ret = SAMPLE_COMM_AUDIO_AencBindAi(AiDev, AiChn, AeChn);
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_DBG(s32Ret);
                return s32Ret;
            }
        }
        printf("Ai(%d,%d) bind to AencChn:%d ok!\n",AiDev , AiChn, AeChn);
    }



    /********************************************
      step 5: start Adec & Ao. ( if you want )
    ********************************************/
    if (HI_TRUE == bSendAdec)
    {
        s32Ret = SAMPLE_COMM_AUDIO_StartAdec(AdChn, gs_enPayloadType);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
            return HI_FAILURE;
        }
		s32AoChnCnt = stAioAttr.u32ChnCnt;
        s32Ret = SAMPLE_COMM_AUDIO_StartAo(AoDev, s32AoChnCnt, &stAioAttr, enInSampleRate, gs_bAioReSample, NULL , 0);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
            return HI_FAILURE;
        }
	//add by mrzhang
	#if 0
	AUDIO_TRACK_MODE_E enTrackMode = AUDIO_TRACK_BOTH_LEFT;
	//enTrackMode = 0;
	AUDIO_TRACK_MODE_E temp;
	s32Ret = HI_MPI_AO_SetTrackMode(AoDev, enTrackMode);
	if (HI_SUCCESS != s32Ret)
	{
	printf("Ao set track mode failure!\n");
	return s32Ret;
	}
	s32Ret = HI_MPI_AO_GetTrackMode(AoDev, &temp);
	if (HI_SUCCESS != s32Ret)
	{
	printf("Ao get track mode failure! AoDev: %d, s32Ret: 0x%x.\n", AoDev,
	s32Ret);
	return s32Ret;
	}
	printf("temp track mode =%d \n",temp);
	#endif
        pfd = SAMPLE_AUDIO_OpenAencFile(AdChn, gs_enPayloadType);
        if (!pfd)
        {
            SAMPLE_DBG(HI_FAILURE);
            return HI_FAILURE;
        }        
        s32Ret = SAMPLE_COMM_AUDIO_CreatTrdAencAdec(AeChn, AdChn, pfd);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
            return HI_FAILURE;
        }

        s32Ret = SAMPLE_COMM_AUDIO_AoBindAdec(AoDev, AoChn, AdChn);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
            return HI_FAILURE;
        }
		printf("bind adec:%d to ao(%d,%d) ok \n", AdChn, AoDev, AoChn);
     }

    printf("\nplease press twice ENTER to exit this sample\n");
    getchar();
    getchar();

    /********************************************
      step 6: exit the process
    ********************************************/
    if (HI_TRUE == bSendAdec)
    {
        s32Ret = SAMPLE_COMM_AUDIO_DestoryTrdAencAdec(AdChn);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
            return HI_FAILURE;
        }
        
        s32Ret = SAMPLE_COMM_AUDIO_StopAo(AoDev, s32AoChnCnt, gs_bAioReSample, HI_FALSE);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
            return HI_FAILURE;
        }
        
        s32Ret = SAMPLE_COMM_AUDIO_StopAdec(AdChn);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
            return HI_FAILURE;
        }
        
        s32Ret = SAMPLE_COMM_AUDIO_AoUnbindAdec(AoDev, AoChn, AdChn);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
            return HI_FAILURE;
        }
        
    }
    
    for (i=0; i<s32AencChnCnt; i++)
    {
        AeChn = i;
        AiChn = i;

        if (HI_TRUE == gs_bUserGetMode)
        {
            s32Ret = SAMPLE_COMM_AUDIO_DestoryTrdAi(AiDev, AiChn);
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_DBG(s32Ret);
                return HI_FAILURE;
            }
        }
        else
        {        
            s32Ret = SAMPLE_COMM_AUDIO_AencUnbindAi(AiDev, AiChn, AeChn);
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_DBG(s32Ret);
                return HI_FAILURE;
            }
        }
    }
    
    s32Ret = SAMPLE_COMM_AUDIO_StopAenc(s32AencChnCnt);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }
    
    s32Ret = SAMPLE_COMM_AUDIO_StopAi(AiDev, s32AiChnCnt, gs_bAioReSample, HI_FALSE);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

/******************************************************************************
* function : Ai -> Ao(with fade in/out and volume adjust)
******************************************************************************/
HI_S32 SAMPLE_AUDIO_AiAo(HI_VOID)
{
    HI_S32 s32Ret;
	HI_S32 s32AiChnCnt;
	HI_S32 s32AoChnCnt;
    AUDIO_DEV   AiDev = SAMPLE_AUDIO_AI_DEV;
    AI_CHN      AiChn = 0;
    AUDIO_DEV   AoDev = SAMPLE_AUDIO_AO_DEV;
    AO_CHN      AoChn = 0;
    AIO_ATTR_S stAioAttr;

#ifdef HI_ACODEC_TYPE_TLV320AIC31
    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = SOUNDMODE;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = SAMPLE_AUDIO_PTNUMPERFRM;
    stAioAttr.u32ChnCnt      = CHNCNT;
    stAioAttr.u32ClkSel      = 1;
#else //  inner acodec
    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = SOUNDMODE;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = SAMPLE_AUDIO_PTNUMPERFRM;
    stAioAttr.u32ChnCnt      = CHNCNT;
    stAioAttr.u32ClkSel      = 0;
#endif
    /* config ao resample attr if needed */
    if (HI_TRUE == gs_bAioReSample)
    {
        stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_32000;
        stAioAttr.u32PtNumPerFrm = SAMPLE_AUDIO_PTNUMPERFRM * 4;

        /* ai 32k -> 8k */
        enOutSampleRate = AUDIO_SAMPLE_RATE_8000;

        /* ao 8k -> 32k */
        enInSampleRate  = AUDIO_SAMPLE_RATE_8000;
    }
    else
    {
        enInSampleRate  = AUDIO_SAMPLE_RATE_BUTT;
    	enOutSampleRate = AUDIO_SAMPLE_RATE_BUTT;
    }
	
    /* resample and anr should be user get mode */
    gs_bUserGetMode = (HI_TRUE == gs_bAioReSample) ? HI_TRUE : HI_FALSE; 
    
    /* config audio codec */
    s32Ret = SAMPLE_COMM_AUDIO_CfgAcodec(&stAioAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }
    
    /* enable AI channle */    
    s32AiChnCnt = stAioAttr.u32ChnCnt;
    s32Ret = SAMPLE_COMM_AUDIO_StartAi(AiDev, s32AiChnCnt, &stAioAttr, enOutSampleRate, gs_bAioReSample, NULL, 0);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }
	HI_MPI_AI_SetVqeVolume(0,0,0x0a);

    /* enable AO channle */   
	s32AoChnCnt = stAioAttr.u32ChnCnt;
    s32Ret = SAMPLE_COMM_AUDIO_StartAo(AoDev, s32AoChnCnt, &stAioAttr, enInSampleRate, gs_bAioReSample, NULL, 0);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }

    /* bind AI to AO channle */
    if (HI_TRUE == gs_bUserGetMode)
    {
        s32Ret = SAMPLE_COMM_AUDIO_CreatTrdAiAo(AiDev, AiChn, AoDev, AoChn);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
            return HI_FAILURE;
        }
    }
    else
    {   
        s32Ret = SAMPLE_COMM_AUDIO_AoBindAi(AiDev, AiChn, AoDev, AoChn);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
            return HI_FAILURE;
        }
    }
    printf("ai(%d,%d) bind to ao(%d,%d) ok\n", AiDev, AiChn, AoDev, AoChn);
	
	if(HI_TRUE == gs_bAoVolumeCtrl)
	{
		s32Ret = SAMPLE_COMM_AUDIO_CreatTrdAoVolCtrl(AoDev);
		if(s32Ret != HI_SUCCESS)
		{
			SAMPLE_DBG(s32Ret);
			return HI_FAILURE;
		}
	}
	
    printf("\nplease press twice ENTER to exit this sample\n");
    getchar();
    getchar();
    
	if(HI_TRUE == gs_bAoVolumeCtrl)
	{
		s32Ret = SAMPLE_COMM_AUDIO_DestoryTrdAoVolCtrl(AoDev);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
            return HI_FAILURE;
        }
	}
	
    if (HI_TRUE == gs_bUserGetMode)
    {
        s32Ret = SAMPLE_COMM_AUDIO_DestoryTrdAi(AiDev, AiChn);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
            return HI_FAILURE;
        }
    }
    else
    {
        s32Ret = SAMPLE_COMM_AUDIO_AoUnbindAi(AiDev, AiChn, AoDev, AoChn);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
            return HI_FAILURE;
        }
    }
    
    s32Ret = SAMPLE_COMM_AUDIO_StopAi(AiDev, s32AiChnCnt, gs_bAioReSample, HI_FALSE);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }
    
    s32Ret = SAMPLE_COMM_AUDIO_StopAo(AoDev, s32AoChnCnt, gs_bAioReSample, HI_FALSE);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }
    
    return HI_SUCCESS;
}


/******************************************************************************
* function : Ai -> Ao
******************************************************************************/
HI_S32 SAMPLE_AUDIO_AiVqeProcessAo(HI_VOID)
{
    HI_S32 i, s32Ret;
    AUDIO_DEV   AiDev = SAMPLE_AUDIO_AI_DEV;
    AUDIO_DEV   AoDev = SAMPLE_AUDIO_AO_DEV;
    HI_S32      s32AiChnCnt;
    HI_S32      s32AoChnCnt;
    AIO_ATTR_S  stAioAttr;
    AI_VQE_CONFIG_S stAiVqeAttr;	
	HI_VOID     *pAiVqeAttr = NULL;
	AO_VQE_CONFIG_S stAoVqeAttr;
	HI_VOID     *pAoVqeAttr = NULL;

#ifdef HI_ACODEC_TYPE_TLV320AIC31
    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = SOUNDMODE;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = SAMPLE_AUDIO_PTNUMPERFRM;
    stAioAttr.u32ChnCnt      = CHNCNT;
    stAioAttr.u32ClkSel      = 1;
#else
    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = SOUNDMODE;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = SAMPLE_AUDIO_PTNUMPERFRM;
    stAioAttr.u32ChnCnt      = CHNCNT;
    stAioAttr.u32ClkSel      = 0;
#endif
    gs_bAioReSample = HI_FALSE;
    enInSampleRate  = AUDIO_SAMPLE_RATE_BUTT;
    enOutSampleRate = AUDIO_SAMPLE_RATE_BUTT;

	if (1 == u32AiVqeType)
    {
	    stAiVqeAttr.s32WorkSampleRate    = AUDIO_SAMPLE_RATE_8000;
    	stAiVqeAttr.s32FrameSample       = SAMPLE_AUDIO_PTNUMPERFRM;
    	stAiVqeAttr.enWorkstate          = VQE_WORKSTATE_COMMON;
    	stAiVqeAttr.bAecOpen             = HI_TRUE;
    	stAiVqeAttr.stAecCfg.bUsrMode    = HI_FALSE;
    	stAiVqeAttr.stAecCfg.s8CngMode   = 0;
    	stAiVqeAttr.bAgcOpen             = HI_TRUE;
    	stAiVqeAttr.stAgcCfg.bUsrMode    = HI_FALSE;
    	stAiVqeAttr.bAnrOpen             = HI_TRUE;
    	stAiVqeAttr.stAnrCfg.bUsrMode    = HI_FALSE;
    	stAiVqeAttr.bHpfOpen             = HI_TRUE;
    	stAiVqeAttr.stHpfCfg.bUsrMode    = HI_TRUE;
    	stAiVqeAttr.stHpfCfg.enHpfFreq   = AUDIO_HPF_FREQ_150;
    	stAiVqeAttr.bRnrOpen             = HI_FALSE;
    	stAiVqeAttr.bEqOpen              = HI_FALSE;
    	stAiVqeAttr.bHdrOpen             = HI_FALSE;
		
		pAiVqeAttr = (HI_VOID *)&stAiVqeAttr;
    }	
	else
	{
		pAiVqeAttr = HI_NULL;
	}

	if (1 == u32AoVqeType)
    {
        memset(&stAoVqeAttr, 0, sizeof(AO_VQE_CONFIG_S));
	    stAoVqeAttr.s32WorkSampleRate    = AUDIO_SAMPLE_RATE_8000;
	    stAoVqeAttr.s32FrameSample       = SAMPLE_AUDIO_PTNUMPERFRM;
	    stAoVqeAttr.enWorkstate          = VQE_WORKSTATE_COMMON;
	    stAoVqeAttr.stAgcCfg.bUsrMode    = HI_FALSE;
	    stAoVqeAttr.stAnrCfg.bUsrMode    = HI_FALSE;
		stAoVqeAttr.stHpfCfg.bUsrMode    = HI_TRUE;
	    stAoVqeAttr.stHpfCfg.enHpfFreq   = AUDIO_HPF_FREQ_150;

		stAoVqeAttr.bAgcOpen = HI_TRUE;
		stAoVqeAttr.bAnrOpen = HI_TRUE;
		stAoVqeAttr.bEqOpen  = HI_TRUE;
		stAoVqeAttr.bHpfOpen = HI_TRUE;
		
		pAoVqeAttr = (HI_VOID *)&stAoVqeAttr;
    }
    

    /********************************************
      step 1: config audio codec
    ********************************************/
    s32Ret = SAMPLE_COMM_AUDIO_CfgAcodec(&stAioAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }

    /********************************************
      step 2: start Ai
    ********************************************/
    s32AiChnCnt = stAioAttr.u32ChnCnt; 
    s32Ret = SAMPLE_COMM_AUDIO_StartAi(AiDev, s32AiChnCnt, &stAioAttr, enOutSampleRate, gs_bAioReSample, pAiVqeAttr, u32AiVqeType);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }

    /********************************************
      step 3: start Ao
    ********************************************/
    s32AoChnCnt = stAioAttr.u32ChnCnt;
    s32Ret = SAMPLE_COMM_AUDIO_StartAo(AoDev, s32AoChnCnt, &stAioAttr, enInSampleRate, gs_bAioReSample, pAoVqeAttr, u32AoVqeType);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }

    /********************************************
      step 4: Ao bind Ai Chn
    ********************************************/
    for (i=0; i<s32AiChnCnt; i++)
    {
        s32Ret = SAMPLE_COMM_AUDIO_CreatTrdAiAo(AiDev, i, AoDev, i);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
            return HI_FAILURE;
        }
		
		printf("bind ai(%d,%d) to ao(%d,%d) ok \n", AiDev, i, AoDev, i);
    }

    printf("\nplease press twice ENTER to exit this sample\n");
    getchar();
    getchar();

    /********************************************
      step 6: exit the process
    ********************************************/
    for (i=0; i<s32AiChnCnt; i++)
    {
        s32Ret = SAMPLE_COMM_AUDIO_DestoryTrdAi(AiDev, i);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
            return HI_FAILURE;
        }
    }
    
    s32Ret = SAMPLE_COMM_AUDIO_StopAi(AiDev, s32AiChnCnt, gs_bAioReSample, HI_TRUE);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }

    s32Ret = SAMPLE_COMM_AUDIO_StopAo(AoDev, s32AoChnCnt, gs_bAioReSample, HI_TRUE);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}


HI_VOID SAMPLE_AUDIO_Usage(HI_VOID)
{
    printf("\n\n/************************************/\n");
    printf("please choose the case which you want to run:\n");
    printf("\t0:  start AI to AO loop\n");
    printf("\t1:  send audio frame to AENC channel from AI, save them\n");
    printf("\t2:  read audio stream from file, decode and send AO\n");
    printf("\t3:  start AI(AEC/ANR/ALC process), then send to AO\n");
    printf("\tq:  quit whole audio sample\n\n");
    printf("sample command:");
}

/******************************************************************************
* function : to process abnormal case
******************************************************************************/
void SAMPLE_AUDIO_HandleSig(HI_S32 signo)
{
    if (SIGINT == signo || SIGTERM == signo)
    {

        SAMPLE_COMM_AUDIO_DestoryAllTrd();
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram exit abnormally!\033[0;39m\n");
    }

    exit(0);
}

/******************************************************************************
* function : main
******************************************************************************/
static int setMicGain(int gain)
{

	HI_MPI_AI_SetVqeVolume(0,0,gain);
	return 0;
	int fdAcodec = open("/dev/acodec", O_RDWR );
		ACODEC_VOL_CTRL input_vol_ctrl;
	int ret;
	input_vol_ctrl.vol_ctrl_mute = 0x0;/*1:mute, 0:unmute*/
	input_vol_ctrl.vol_ctrl = gain;
	if( ioctl( fdAcodec, ACODEC_SET_ADCL_VOL, &input_vol_ctrl ) )
	{
		printf( "ioctl err!\n" );
		return -1;
	}
    close( fdAcodec );
	return 0;
}

HI_S32 main(int argc, char *argv[])
{
    HI_S32 s32Ret = HI_SUCCESS;
    HI_CHAR ch;
    HI_BOOL bExit = HI_FALSE;
  
    
    signal(SIGINT, SAMPLE_AUDIO_HandleSig);
    signal(SIGTERM, SAMPLE_AUDIO_HandleSig);
      VB_CONF_S stVbConf;
    memset(&stVbConf, 0, sizeof(VB_CONF_S));
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: system init failed with %d!\n", __FUNCTION__, s32Ret);
        return HI_FAILURE;
    }
 
	/**
	 1.设置音量
	 * ***/

    /******************************************
     1 choose the case
    ******************************************/
    while (1)
    {
        SAMPLE_AUDIO_Usage();
        ch = (char)getchar();
        getchar();
        switch (ch)
        {
            case '0':
            {
                SAMPLE_AUDIO_AiAo();
                break;
            }
            case '1':
            {
                SAMPLE_AUDIO_AiAenc();
                break;
            }
            case '2':
            {
                SAMPLE_AUDIO_AdecAo();
                break;
            }
            case '3':
            {
                SAMPLE_AUDIO_AiVqeProcessAo();
                break;
            }
            case 'q':
            case 'Q':
            {
                bExit = HI_TRUE;
                break;
            }
            default :
            {
                printf("input invaild! please try again.\n");
                break;
            }
        }
        
        if (bExit)
        {
            break;
        }
    }
    
    SAMPLE_COMM_SYS_Exit();
    
    return s32Ret;
}


