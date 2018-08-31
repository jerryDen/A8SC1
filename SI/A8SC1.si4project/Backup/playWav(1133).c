#include "common/wav.h"
#include "common/playWav.h"
#include "common/debugLog.h"

int RingVol = 80;
void palyWavInit(void)
{
	
	 LOGD("RINGVOL:%d\n",RingVol);
}
void palyRing(void)
{
	wavPlaymusic(ringFlie, 60,RingVol);
}
void palyStartingUp(void)
{
	wavPlaymusic(startingUp, 0,RingVol);
}

void palyKeyTone(void)
{
	
	wavPlaymusic(keyTone, 0,RingVol);
}

void palyDetectAlarm(void)
{
	wavPlaymusic(detectAlarm, 0,RingVol);
}
void palyOpenDoorSucceed(void)
{
	wavPlaymusic(openDoor, 0,RingVol);
}


void palyLineBusy(void)
{
	wavPlaymusic(lineBusy, 0,RingVol);
}
void playLayoutAlarm(void)
{
	wavPlaymusic(layoutAlarm,0,RingVol);
}
void playRepealAlarm(void)
{
	wavPlaymusic(repealAlarm,0,RingVol);
}
void palyAlarm(void)
{
	wavPlaymusic(alarmRing,20,RingVol);
}
void palyDelayAlarm(void)
{
	wavPlaymusic(delayAlarm, 0,RingVol);
}

void palyoPerationSuccess(void) 
{
	wavPlaymusic(operationSuccess, 0,RingVol);
}


void palyLineFault(void)
{
	wavPlaymusic(lineFault, 0,RingVol);
}
void palyPeerHangUp(void)
{
	wavPlaymusic(peerHangUp, 0,RingVol);
}

void palyTalkTimeout(void)
{
	wavPlaymusic(talkTimeout, 0,RingVol);
}
void palyNoAnswer(void)
{
	wavPlaymusic(noAnswer, 0,RingVol);
}
void stopPlayWav(void)
{
	wavStopPlay();
}
