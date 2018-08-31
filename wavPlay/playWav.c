#include "common/wav.h"
#include "common/playWav.h"
#include "common/debugLog.h"

int RingVol = 80;

void playStartingUp(void)
{
	wavPlaymusic(startingUp, 0,RingVol);
}

void playCardpastdue(void)
{

	wavPlaymusic(cardpastdue, 0,RingVol);

}
void playCardUnregistered(void)
{
	wavPlaymusic(cardUnregistered, 0,RingVol);

}
void playOpenTheDoor(void)
{
	wavPlaymusic(openTheDoor, 0,RingVol);

}

void stopPlayWav(void)
{
	wavStopPlay();
}
