#ifndef DEBUGLOG_H
#define DEBUGLOG_H
#include <android/log.h>
#if 0
#ifdef FOR_LOCAL_CLIENT
#define LOG    "WB-A8Talk-Client_L" // 这个是自定义的LOG的标识
#else
#define LOG    "WB-A8Talk-Client_G" // 这个是自定义的LOG的标识
#endif
#endif
#define LOG    "WB-A8HardWare-Control"
#ifndef DBG_LEVEL
#define DBG_LEVEL 5 //set DBG_LEVEL 0
#endif
#if DBG_LEVEL > 4
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG,__VA_ARGS__) // 定义LOGD类型
#else
#define LOGD(...) do{}while(0)
#endif
#if DBG_LEVEL >3
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG,__VA_ARGS__) // 定义LOGI类型
#else
#define LOGI(...) do{}while(0)
#endif

#if DBG_LEVEL >2
#define LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG,__VA_ARGS__) // 定义LOGW类型
#else
#define LOGW(...) do{}while(0)
#endif

#if DBG_LEVEL >1
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG,__VA_ARGS__) // 定义LOGE类型
#else
#define LOGE(...) do{}while(0)
#endif

#if DBG_LEVEL >0
#define LOGF(...)  __android_log_print(ANDROID_LOG_FATAL,LOG,__VA_ARGS__) // 定义LOGF类型
#else
#define LOGF(...) do{}while(0)
#endif

#endif
