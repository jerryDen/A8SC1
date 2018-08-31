#ifndef _PLAY_WAV_H_
#define _PLAY_WAV_H_




#define WAV_DIR			"/usr/work/wav" 

#define startingUp 			WAV_DIR"/startingUp.wav"


#define cardUnregistered  	WAV_DIR"/cardUnregistered.wav"
#define cardpastdue  		WAV_DIR"/cardpastdue.wav"
#define openTheDoor  		WAV_DIR"/openTheDoor.wav"



#define DefaultVolume      	90




void palyCardpastdue(void);
void palyCardUnregistered(void);
void palyOpenTheDoor(void);

void palyStartingUp(void);




#endif

