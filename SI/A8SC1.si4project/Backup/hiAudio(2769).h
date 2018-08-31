#ifndef _HI_AUDIO_H_
#define _HI_AUDIO_H_

#include "hi_type.h"
#include "hi_comm_aio.h"
#include "wav.h"

#define FORMAT	AUDIO_BIT_WIDTH_8	
#define CHANNELS	1
#define RATE	AUDIO_SAMPLE_RATE_8000

/*
  用户设置范围0<-->100，芯片设置范围7F<--->0，用户值为0时，设置对应芯片值0x7f，静音，
用户值1--100对应到芯片值99--0，芯片值100--126丢弃不用
*/
#define DEFAULT_VOLUME	90
#define MAX_VOLUME		100


//hi_audio.c

int talkPthreadEncodeInit(void);
//设置播放时的音量
int setPlaybackVolume(int val);
int audioRwlockInit(void);

HI_S32 wavInitPbPcm( AUDIO_BIT_WIDTH_E format, HI_U32 channels, HI_U32 rate, int volume );
HI_VOID wavExitPbPcm(HI_VOID);
HI_U32 GetWavChunkBytes(HI_VOID);
HI_S32 wavWritePcm( HI_VOID *buf, HI_U32 bytes );
HI_S32 us_wavWritePcm( HI_VOID *buf, HI_U32 bytes );
HI_S32 us_wavInitPbPcm(AUDIO_BIT_WIDTH_E format, HI_U32 channels, int volume );

HI_S32 talkInitPcm(HI_VOID);
HI_S32 us_talkInitPcm(HI_VOID);

HI_VOID talkExitPcm(HI_VOID);
HI_S32 talkGetPcm(HI_VOID **BufAddr,HI_U32 *BufLen );
HI_U32 talkWritePcm( HI_VOID *buf, HI_U32 bytes );


//hi_wav.c
//void playbackWave(char *name, int sec, int volume);
#define playbackWave(pName, sec, volume)	wavePlayStart(pName, sec, volume)

/*
此函数未检查通话状态，用于播放通话铃声或通话相关提示
  在收到呼叫进入<WAIT_LOCAL_HOOK>状态时，需要播放铃声；收到摘机进入<TALKING>状态，或结束通话
时需要关闭通话铃声，这时通话状态就不为<TALK_FREE>
*/
void playbackTalkPromptWave(char *name, int sec, int volume);
void playbackWavePrompt(char *name, int sec, int volume);
void playbackKeyTone(void);
void playbackKeyNum(int numAscii);



//audioProcess.c
void send_HI_AudioFrame(void *buf, uint32_t size);


int audioFromConnectCreate(unsigned long aSource_ip, int clientIdx);
int souceQuitAudioFromConnect(unsigned long aSource_ip);
int clientQuitAudioFromConnect(unsigned long aSource_ip, int clientIdx);

void * pthreadAudioDecode(void *arg);



//fm1188.c
void initFM1188();


#endif
