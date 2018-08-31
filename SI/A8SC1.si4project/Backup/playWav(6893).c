#include "common/wav.h"
#include "common/playWav.h"
#include "common/debugLog.h"

int RingVol = 80;

void palyStartingUp(void)
{
	wavPlaymusic(startingUp, 0,RingVol);
}

void palyCardpastdue(void)
{

	wavPlaymusic(cardpastdue, 0,RingVol);

}
void palyCardUnregistered(void)
{
	wavPlaymusic(cardUnregistered, 0,RingVol);

}
void palyOpenTheDoor(void)
{
	wavPlaymusic(openTheDoor, 0,RingVol);

}

void stopPlayWav(void)
{
	wavStopPlay();
}
