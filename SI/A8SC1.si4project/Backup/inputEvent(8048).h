#ifndef _INPUT_EVENT_H_
#define _INPUT_EVENT_H_

#include <linux/input.h>

typedef enum E_KEY_STATE{
	E_SHORT_PRESS = 0,
	E_LONG_PRESS,
	E_ALWAYS_PRESS,	
	E_INVALID,
}E_KEY_STATE;

#define CALL_MANAGER    4
#define HUANG_UP        3
#define OPEN_LOCK       2


#define MQ_ALARM        KEY_A   //煤气
#define HW_ALARM		KEY_D   //红外
#define YW_ALARM        KEY_B   //烟雾 
#define JJ_ALARM        KEY_C   //紧急  
#define MC_ALARM        KEY_E   //门磁


#define KEY_STATE_UP     0
#define KEY_STATE_DOWN   1


typedef void (*CallBackFuntion)(int, int,int);
int keypadInit(CallBackFuntion KeypadProcess);
int closeGpioKeyDev();



#endif

