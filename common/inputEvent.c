#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <assert.h>
#include <linux/input.h>
#include <termios.h>
#include <stdint.h>

#include <netinet/in.h>
#include "threadManage.h"
#include "inputEvent.h"

#define DEBUG_GPIO_KEY	0

int iPadIsOn = 0; //平板是否放在底座上

typedef struct KeyRecord{
	int keyCode;
	unsigned long long int  keyTime;
	int isUser;
}KeyRecord;


/*
 注意: 红外、煤气、紧急，3个有线安防sensor的检查，放在gpio_keys驱动中了，
 相当于一个按键，目前只支持短路报警(相当于按键压下)
 */
static void *keyScanThread(void *arg);
static int findUnusedKeyArray(void);
static int findkeyCodeIndex(int keyCode);


CallBackFuntion callBackKeypadProcess = NULL;

/*
 * 按键初始化：
 *
 * KeypadProcess:按键处理函数，获取值处理相应事情
 *
 * */
//static Thread_ID keyScanTid;
static pThreadOps keyScanTid;

static KeyRecord keyArray[6] = {0};

int keypadInit(CallBackFuntion KeypadProcess) {

	openInputDev();
//	keyScanTid = pthread_classCreate(keyScanThread,NULL,0,NULL);
//pthread_start(keyScanTid);
	keyScanTid = pthread_register(keyScanThread,NULL,0,NULL);
	
	keyScanTid->start(keyScanTid);
	
	callBackKeypadProcess = KeypadProcess;
	return 0;
}

#define INPUT_DEV_PATH	"/dev/input"	//"/dev/input"
static int gpio_keys_fd = -1, touch_fd = -1;

int openInputDev() {
	int fd;
	char name[256] = "unknown";

	fd = open(INPUT_DEV_PATH"/event0", O_RDONLY | O_NOCTTY | O_NONBLOCK);
	if (fd < 0) {
		printf("(*)can't open %s/event0\n", INPUT_DEV_PATH);
	} else {
		ioctl(fd, EVIOCGNAME(sizeof(name)), name);
//		printf("event0 dev name: %s\n", name);

		if (strcmp(name, "gpio_keys") == 0) {
			gpio_keys_fd = fd;
			printf("(*)kepad dev in %s/event0\n", INPUT_DEV_PATH);
		} else if (strcmp(name, "cp2007_ts") == 0) {
			touch_fd = fd;
			printf("(*)touch dev in %s/event0\n", INPUT_DEV_PATH);
		}
	}

	fd = open(INPUT_DEV_PATH"/event1", O_RDONLY | O_NOCTTY | O_NONBLOCK);
	if (fd < 0) {
		printf("(*)can't open %s/event1\n", INPUT_DEV_PATH);
	} else {
		ioctl(fd, EVIOCGNAME(sizeof(name)), name);
//		printf("event1 dev name: %s\n", name);

		if (strcmp(name, "gpio_keys") == 0) {
			gpio_keys_fd = fd;
			printf("(*)kepad dev in %s/event1\n", INPUT_DEV_PATH);
		} else if (strcmp(name, "cp2007_ts") == 0) {
			touch_fd = fd;
			printf("(*)touch dev in %s/event0\n", INPUT_DEV_PATH);
		}
	}

	if (gpio_keys_fd < 0)
		return -1;
	else
		return 0;
}

int read_gpio_key(void ) {
	int i, r,j;
	struct input_event ev[64];
	static struct timespec  current_time;
	unsigned long long int  new_nsec;
	unsigned long long int  press_nsec;
	E_KEY_STATE keyState = E_SHORT_PRESS;
	fd_set fds;
	struct timeval tv;
	FD_ZERO(&fds);
	FD_SET(gpio_keys_fd, &fds);
	tv.tv_sec = 0;
	tv.tv_usec = 10*1000; //注意:不能大于喂狗时间
	//检查指定的文件是否可读
	r = select(gpio_keys_fd + 1, &fds, NULL, NULL, NULL);
	if (r > 0) {
		r = read(gpio_keys_fd, ev, sizeof(struct input_event) * 64);
		if (r <= 0)
			goto no_key;
#if DEBUG_GPIO_KEY
		//每次按键均能读到2个事件，因为每次按键事件后都会发一个EV_SYN的同步事件
		printf("key read bytes:%d, size:%d\n", r, sizeof(struct input_event));
#endif

		for (i = 0; i < r / sizeof(struct input_event); i++) {
#if DEBUG_GPIO_KEY
			printf("key type:%d, val:%d, code:%d\n", ev[i].type, ev[i].value, ev[i].code);
#endif
			if (ev[i].type == EV_KEY) {
				clock_gettime(CLOCK_MONOTONIC, &current_time);
				if(ev[i].value == KEY_STATE_DOWN){
					
					j = findUnusedKeyArray();
					if(j == -1)
						return -1;
					keyArray[j].keyTime = ((unsigned long long int )(current_time.tv_sec)*1000000000LL + current_time.tv_nsec);
					keyArray[j].isUser = 1;
					keyArray[j].keyCode = ev[i].code;
					
				}else if(ev[i].value == KEY_STATE_UP ){
					j = findkeyCodeIndex(ev[i].code);
					if(j == -1)
						return -1;
					keyArray[j].isUser = 0;
					keyArray[j].keyCode = 0;
					new_nsec = ((unsigned long long int )(current_time.tv_sec)*1000000000LL + current_time.tv_nsec);
					press_nsec = new_nsec -keyArray[j].keyTime;
					if(press_nsec <= 600*1000*1000LL  )
					{
						keyState = E_SHORT_PRESS;
					}else if((press_nsec >=2000*1000*1000LL) && (press_nsec <=7000*1000*1000LL))
					{
						keyState = E_LONG_PRESS;
					}else if( (press_nsec >=10000*1000*1000LL))
					{
						keyState = E_ALWAYS_PRESS;
					}else
						keyState = E_INVALID;
				}
				callBackKeypadProcess(ev[i].code, ev[i].value,keyState);
			}
		}
	}
	no_key: return -1;
}
static int findUnusedKeyArray(void)
{
	int i;
	for(i = 0;i<sizeof(keyArray)/sizeof(keyArray[0]);i++ )
	{
		if(keyArray[i].isUser == 0)
			return i;
	}
	return -1;
}
static int findkeyCodeIndex(int keyCode)
{
	int i;
	for(i = 0;i<sizeof(keyArray)/sizeof(keyArray[0]);i++ )
	{
		if(keyArray[i].isUser &&(keyArray[i].keyCode == keyCode))
			return i;
	}
	return -1;
}
int closeGpioKeyDev() {
	close(gpio_keys_fd);
	return 0;
}

static void *keyScanThread(void *arg)
{
	while(keyScanTid->check(keyScanTid)==Thread_Run)
	{
		read_gpio_key();
	}
	printf(" keyScanThread exit !\n");
	return NULL;		
}



