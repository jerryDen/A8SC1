#ifndef _WAV_ALSA_AUDIO_H_
#define _WAV_ALSA_AUDIO_H_

#include "hi_comm_aio.h"
#include "hiAudio.h"


#define SND_PCM_FORMAT_U8		AUDIO_BIT_WIDTH_8
#define SND_PCM_FORMAT_S16_LE	AUDIO_BIT_WIDTH_16
#define SND_PCM_FORMAT_S24_3LE	AUDIO_BIT_WIDTH_16   //AUDIO_BIT_WIDTH_32
#define SND_PCM_FORMAT_S24_LE	AUDIO_BIT_WIDTH_16//AUDIO_BIT_WIDTH_32
#define SND_PCM_FORMAT_S32_LE	AUDIO_BIT_WIDTH_16//AUDIO_BIT_WIDTH_32
#define SND_PCM_FORMAT_FLOAT_LE	AUDIO_BIT_WIDTH_16 //AUDIO_BIT_WIDTH_32


#define snd_pcm_format_size( format, samples) \
	(format == AUDIO_BIT_WIDTH_8) ? samples : \
	(format == AUDIO_BIT_WIDTH_16) ? (2 * samples) : (4 * samples)
		

#endif

