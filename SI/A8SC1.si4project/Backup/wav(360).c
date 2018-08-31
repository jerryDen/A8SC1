#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <linux/limits.h>
#include <time.h>
#include <locale.h>
#include <assert.h>
#include <sys/poll.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <asm/byteorder.h>
#include <pthread.h>
#include <stdint.h>
#include "common/threadManage.h"
#include "common/formats.h"
#include "common/hiAudio.h"
#include "common/wavAlsa.h"
#include "common/DebugLog.h"
#include "common/commonHead.h"




typedef struct WavPlayArg{
	char *name;
	int playTime;
	int volume;
}WavPlayArg,*pWavPlayArg;
typedef struct WavParames{
	int format;
	unsigned int channels;
	unsigned int rate;
	unsigned int vol;
	unsigned int pb_data_bytes;
} WavParames,*pWavParames;


//static Thread_ID wavThreadID;
static pThreadOps wavThreadID;
static pthread_mutex_t wavThreadID_mutex = PTHREAD_MUTEX_INITIALIZER;



static void *playWavThread(void * arg);
static ssize_t safe_read(int fd, void *buf, size_t count);
static ssize_t test_wavefile_read(int fd, u_char *buffer, size_t *size, size_t reqsize, int line);
static size_t test_wavefile(int fd, u_char *_buffer, size_t size,pWavParames wavParames );

//唤醒线程进行播放
//playTime == 0 只播放一次 
int wavPlaymusic(const char *name,int playTime,int volume)
{
	
	 WavPlayArg wavPlayarg ;
	 wavPlayarg.name = name;
	 wavPlayarg.playTime = playTime;
	 wavPlayarg.volume = volume;
	 pthread_mutex_lock(&wavThreadID_mutex);
	 if(wavThreadID != NULL)
	 	pthread_destroy(&wavThreadID);
	 wavThreadID =  pthread_register(playWavThread,&wavPlayarg,sizeof(WavPlayArg),NULL);
	 if( wavThreadID == NULL){
	 	pthread_mutex_unlock(&wavThreadID_mutex);
	 	goto fail0;
	 }
	
	wavThreadID->start(wavThreadID);
	pthread_mutex_unlock(&wavThreadID_mutex);
	 return 0;
fail0:
	 return -1;
	
}

int wavStopPlay(void)
{
	pthread_mutex_lock(&wavThreadID_mutex);
	pthread_destroy(&wavThreadID);
	pthread_mutex_unlock(&wavThreadID_mutex);
}
int wavExit(void)
{	
	pthread_mutex_lock(&wavThreadID_mutex);
	 pthread_destroy(&wavThreadID);
	 pthread_mutex_unlock(&wavThreadID_mutex);
	return 0;
}
static unsigned int  calc_count(int sec,WavParames wavParames )
{
	int count;
	int pb_count;

	if (sec) {
		//先计算每秒字节数
		count = snd_pcm_format_size(wavParames.format, wavParames.rate * wavParames.channels);
		pb_count = count * sec;

	}
	else
		pb_count = wavParames.pb_data_bytes;
	return pb_count;
}

static void init_raw_data(pWavParames wavParames )
{
	//裸数据没有格式，只能使用默认的参数
	wavParames->format = FORMAT;
	wavParames->channels = CHANNELS;
	wavParames->rate = RATE;
	wavParames->vol = DEFAULT_VOLUME;
}

static void *playWavThread(void * arg)
{
	
	
	pWavPlayArg wavArg = (pWavPlayArg)arg;
	int pb_fd = -1;
	int ret;
	WavParames wavParames;
	unsigned char *wav_pb_buf = malloc(1024);
	int dtawave,pb_count,loaded;
	unsigned int  written = 0,r,c;
	unsigned int chunk_bytes;
	bzero(wav_pb_buf,1024);
	if(arg == NULL)
		return NULL;
	if ((pb_fd = open64(wavArg->name, O_RDONLY, 0)) == -1) {
			perror(wavArg->name);
			goto end;
	}
	if ((unsigned int)safe_read(pb_fd, wav_pb_buf, sizeof(WaveHeader)) != sizeof(WaveHeader)) {
		LOGE("read error");
		goto end;
	}
	if ((dtawave = test_wavefile(pb_fd, wav_pb_buf, sizeof(WaveHeader),&wavParames)) >= 0) {
		pb_count = calc_count(wavArg->playTime,wavParames );
		loaded = dtawave;
		wavParames.vol = wavArg->volume;
	
	}else{
		init_raw_data(&wavParames);
		pb_count = calc_count(wavArg->playTime,wavParames);
		loaded = sizeof(WaveHeader);
		wavParames.vol = wavArg->volume;
				
	}
	if(wavInitPbPcm(wavParames.format,AUDIO_SAMPLE_RATE_8000,wavParames.channels,wavParames.vol))
			goto end;
	wav_pb_buf = realloc(wav_pb_buf, GetWavChunkBytes());
	if (wav_pb_buf == NULL) {
		LOGE("not enough memory");
		goto end;
	}
start:
	system("himm 0x20180200  0x80");


	chunk_bytes = GetWavChunkBytes();
	//while(pthread_checkRunState(wavThreadID) == Thread_Run){
	while(wavThreadID&&wavThreadID->check(wavThreadID) == Thread_Run){
		if(written >= pb_count){
			break;
		}
		if( loaded > chunk_bytes ) {
			if( wavWritePcm(wav_pb_buf + written, chunk_bytes ) < 0) 
				goto end;
			written += chunk_bytes;
			loaded -= chunk_bytes;
			continue;
		}
		else if( written > 0 && loaded > 0 ){
			memmove( wav_pb_buf, wav_pb_buf + written, loaded );
		}
		do {
			//每次播放一个chunk字节数，减去原来剩下的，c是还需要从文件中读取的字节数
			c = pb_count - written;
			if( c > chunk_bytes )
				c = chunk_bytes;
			c -= loaded;

			if( c == 0 )
				break;
			r = safe_read( pb_fd, wav_pb_buf + loaded, c );
			if (r < 0) {
				goto end;
			}
			if (r == 0){
			//	printf("wav file end, mute bytes: %u\n", (unsigned int)c);
				//返回到文件起始有效数据位置，准备从头再开始播放
				lseek( pb_fd, -wavParames.pb_data_bytes, SEEK_CUR);
				break;
			}
			else loaded += r;
		} while( loaded < chunk_bytes );
		if(wavWritePcm( wav_pb_buf, loaded)<0 )
				goto end;
		written += loaded;
		loaded = 0;
	
	}
end:
	system("himm 0x20180200  0x00");
	//usAudioClose();
	LOGD("playWavThread exit!");
	if(wav_pb_buf)
		free(wav_pb_buf);
	if (pb_fd > 0){
		close(pb_fd);
		pb_fd = -1;
	}
	return NULL;
}
static ssize_t safe_read(int fd, void *buf, size_t count)
{
	ssize_t result = 0, res;

	while (count > 0) {
		if ((res = read(fd, buf, count)) == 0)
			break;
		if (res < 0)
			return result > 0 ? result : res;
		count -= res;
		result += res;
		buf = (char *)buf + res;
	}
	return result;
}

static ssize_t test_wavefile_read(int fd, u_char *buffer, size_t *size, size_t reqsize, int line)
{
	if (*size >= reqsize)
		return *size;
	if ((size_t)safe_read(fd, buffer + *size, reqsize - *size) != reqsize - *size) {
		LOGE( "read error (called from line %i)", line);
		return -1;
	}
	return *size = reqsize;
}

#define check_wavefile_space(buffer, len, blimit) \
	if (len > blimit) { \
		blimit = len; \
		if ((buffer = realloc(buffer, blimit)) == NULL) { \
			LOGE( "not enough memory");		  \
			return -1; \
		} \
	}

/*
 * test, if it's a .WAV file, > 0 if ok (and set the speed, stereo etc.)
 *                            == 0 if not
 * Value returned is bytes to be discarded.
  检查是否为WAV文件，如果是，则读出参数:采样格式，通道数，采样率，检查它们并赋值到
全局变量hwparams，然后寻找第一个数据chunk，找到后就退出函数，下面就可以进行播放了
参数
fd 是要播放的文件描述符，已在调用此函数前打开
_buffer 是在调用此函数前，已经从文件中读出的若干数据的指针，注意前面有个下划线
size 是_buffer中数据的大小
返回
-1 不是wav文件或文件有错误
0或正整数 调用此函数前已从fd中读出了若干数据，函数执行过程中还会读出一些数据，
	      将这些数据去除头部后就是实际的音频数据，是需要播放的数据，需要调用者去处理，
	      这里返回的就是这些需要处理的数据大小，数据会移到_buffer的最前端。
	      其实使用lseek()函数应该更简单，但lseek()函数好像不支持管道等操作。
*/
static size_t test_wavefile(int fd, u_char *_buffer, size_t size,pWavParames wavParames )
{
	WaveHeader *h = (WaveHeader *)_buffer;
	u_char *buffer = NULL;
	size_t blimit = 0;
	WaveFmtBody *f;
	WaveChunkHeader *c;
	uint type, len;
	//检查WaveHeader，判断是否wave文件
	if (size < sizeof(WaveHeader))
		return -1;
	if (h->magic != WAV_RIFF || h->type != WAV_WAVE)
		return -1;
	if (size > sizeof(WaveHeader)) {
		check_wavefile_space(buffer, size - sizeof(WaveHeader), blimit);
		memcpy(buffer, _buffer + sizeof(WaveHeader), size - sizeof(WaveHeader));
	}
	size -= sizeof(WaveHeader);

	//WaveHeader ok，下面寻找type为WAT_FMT的WaveChuncHeader
	while (1) {
		check_wavefile_space(buffer, sizeof(WaveChunkHeader), blimit);
		if( test_wavefile_read(fd, buffer, &size, sizeof(WaveChunkHeader), __LINE__) < 0)
			goto fail;
		c = (WaveChunkHeader*)buffer;
		type = c->type;
		len = LE_INT(c->length);
		len += len % 2;	//WaveFmtBody结构为short整形对齐
		if (size > sizeof(WaveChunkHeader))
			memmove(buffer, buffer + sizeof(WaveChunkHeader), size - sizeof(WaveChunkHeader));
		size -= sizeof(WaveChunkHeader);
		if (type == WAV_FMT)
			break;
		check_wavefile_space(buffer, len, blimit);
		if( test_wavefile_read(fd, buffer, &size, len, __LINE__) < 0)
			goto fail;

		if (size > len)
			memmove(buffer, buffer + len, size - len);
		size -= len;
	}
	//检查WaveFmtBody格式，获取文件格式参数，并检查它们
	if (len < sizeof(WaveFmtBody)) {
		LOGE( "unknown length of 'fmt ' chunk (read %u, should be %u at least)",
				len, (u_int)sizeof(WaveFmtBody));
		goto fail;;
	}
	check_wavefile_space(buffer, len, blimit);
	if (test_wavefile_read(fd, buffer, &size, len, __LINE__) < 0)
		goto fail;

	f = (WaveFmtBody*) buffer;
	if (LE_SHORT(f->format) == WAV_FMT_EXTENSIBLE) {
		WaveFmtExtensibleBody *fe = (WaveFmtExtensibleBody*)buffer;
		if (len < sizeof(WaveFmtExtensibleBody)) {
			LOGE( "unknown length of extensible 'fmt ' chunk (read %u, should be %u at least)",
					len, (u_int)sizeof(WaveFmtExtensibleBody));
			goto fail;
		}
		if (memcmp(fe->guid_tag, WAV_GUID_TAG, 14) != 0) {
			LOGE( "wrong format tag in extensible 'fmt ' chunk");
			goto fail;
		}
		f->format = fe->guid_format;
	}

	if (LE_SHORT(f->format) != WAV_FMT_PCM && LE_SHORT(f->format) != WAV_FMT_IEEE_FLOAT) {
		LOGE( "can't play WAVE-file format 0x%04x which is not PCM or FLOAT encoded", LE_SHORT(f->format));
		goto fail;
	}
	if (LE_SHORT(f->channels) < 1) {
		LOGE( "can't play WAVE-files with %d tracks", LE_SHORT(f->channels));
		goto fail;
	}
	(*wavParames).channels = LE_SHORT(f->channels);
	switch (LE_SHORT(f->bit_p_spl)) {
	
	case 8:
//		fprintf(stderr, "Warning: format is changed to U8\n");
		(*wavParames).format = SND_PCM_FORMAT_U8;
		break;
	case 16:
//		fprintf(stderr, "Warning: format is changed to S16_LE\n");
		(*wavParames).format = SND_PCM_FORMAT_S16_LE;
		break;
	case 24:
		switch (LE_SHORT(f->byte_p_spl) / wavParames->channels) {
		case 3:
//			fprintf(stderr, "Warning: format is changed to S24_3LE\n");
			(*wavParames).format = SND_PCM_FORMAT_S24_3LE;
			break;
		case 4:
//			fprintf(stderr, "Warning: format is changed to S24_LE\n");
			(*wavParames).format = SND_PCM_FORMAT_S24_LE;
			break;
		default:
			LOGE( " can't play WAVE-files with sample %d bits in %d bytes wide (%d channels)",
			      LE_SHORT(f->bit_p_spl), LE_SHORT(f->byte_p_spl), (*wavParames).channels);
			goto fail;
		}
		break;
	case 32:
        if (LE_SHORT(f->format) == WAV_FMT_PCM)
			(*wavParames).format = SND_PCM_FORMAT_S32_LE;
        else if (LE_SHORT(f->format) == WAV_FMT_IEEE_FLOAT)
			(*wavParames).format = SND_PCM_FORMAT_FLOAT_LE;
		break;
	default:
		LOGE( " can't play WAVE-files with sample %d bits wide", LE_SHORT(f->bit_p_spl));
		goto fail;
	}
	(*wavParames).rate = LE_INT(f->sample_fq);

	if (size > len)
		memmove(buffer, buffer + len, size - len);
	size -= len;

	//寻找第一个数据块，每个chunk前面都有一个WaveChunkHeader头
	while (1) {
		u_int type, len;
		check_wavefile_space(buffer, sizeof(WaveChunkHeader), blimit);
		if( test_wavefile_read(fd, buffer, &size, sizeof(WaveChunkHeader), __LINE__) < 0)
			goto fail;

		c = (WaveChunkHeader*)buffer;
		type = c->type;
		len = LE_INT(c->length);
		if (size > sizeof(WaveChunkHeader))
			memmove(buffer, buffer + sizeof(WaveChunkHeader), size - sizeof(WaveChunkHeader));
		size -= sizeof(WaveChunkHeader);
		if (type == WAV_DATA) {
			//文件有效播放数据的总长度赋值到全局变量pb_data_bytes，这个是没有包含头部的长度
			(*wavParames).pb_data_bytes = len;
#if DEBUG_WAVE
	printf("wave file len bytes = %u\n", len);
#endif
			//如果读出的数据中还有需要播放的数据，移到_buffer的最前端，由调用者去播放
			if (size > 0)
				memcpy(_buffer, buffer, size);
			free(buffer);
			return size;	//返回的是已从文件中读出，去除头部后需要播放的数据长度
		}
		len += len % 2;
		check_wavefile_space(buffer, len, blimit);
		if( test_wavefile_read(fd, buffer, &size, len, __LINE__) < 0)
			goto fail;

		if (size > len)
			memmove(buffer, buffer + len, size - len);
		size -= len;
	}

fail:
	/* shouldn't be reached */
	if(buffer)
		free(buffer);
	return -1;
}
















