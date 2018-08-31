#ifndef _PLAY_WAV_H_
#define _PLAY_WAV_H_




#define WORK_DIR			"/usr/work" 
#define PICTURE_DIR			WORK_DIR"/picture"
#define WAV_DIR 			WORK_DIR"/wav"
#define FONT_DIR			WORK_DIR"/font"
#define RECORD_DIR			WORK_DIR"/record"
#define AV_RECORD_DIR 		RECORD_DIR"/talk_av"			//留言留言保存目录
#define SM_PUBLIC_DIR 		RECORD_DIR"/sm_public"		//公共短信保存目录
#define SM_PRIVATE_DIR 		RECORD_DIR"/sm_private"		//个人短信保存目录
#define SECURITY_DIR		WORK_DIR"/security"
#define X6B_advertise 		WORK_DIR"/X6B_advertise"
#define startingUp 			WAV_DIR"/startingUp.wav"
#define ringFlie  			WAV_DIR"/ring.wav"
#define keyTone   			WAV_DIR"/keyTone.wav"
#define openDoor  			WAV_DIR"/openDoor.wav"
#define lineBusy  			WAV_DIR"/lineBusy.wav"
#define delayAlarm 			WAV_DIR"/alarmDelay.wav"
#define layoutAlarm			WAV_DIR"/layoutAlarm.wav"
#define repealAlarm			WAV_DIR"/repealAlarm.wav"
#define operationSuccess 	WAV_DIR"/operationSuccess.wav"
#define lineFault  			WAV_DIR"/lineFault.wav"
#define peerHangUp  		WAV_DIR"/peerHangUp.wav"
#define alarmRing  			WAV_DIR"/alarm.wav"
#define talkTimeout  		WAV_DIR"/talkTimeout.wav"
#define noAnswer  			WAV_DIR"/noAnswer.wav"
#define detectAlarm         WAV_DIR"/detectAlarm.wav"

#define DefaultVolume      	90


void palyWavInit(void);
void palyRing(void);
void palyKeyTone(void);
void palyPeerHangUp(void);

void palyLineBusy(void);
void palyLineFault(void);
void palyTalkTimeout(void);
void palyNoAnswer(void);
void stopPlayWav(void);
void palyStartingUp(void);
void palyOpenDoorSucceed(void);
void palyAlarm(void);
void palyDelayAlarm(void);
void palyoPerationSuccess(void);
void palyDetectAlarm(void);



#endif

