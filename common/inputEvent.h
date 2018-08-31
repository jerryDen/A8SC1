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



#define  OPEN_DOOR       KEY_A   //紧急  



#define KEY_STATE_UP     0
#define KEY_STATE_DOWN   1


typedef void (*CallBackFuntion)(int, int,int);
int keypadInit(CallBackFuntion KeypadProcess);
int closeGpioKeyDev();



#endif

