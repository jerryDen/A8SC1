#ifndef DEBUGLOG_H
#define DEBUGLOG_H

#define LOGB      " "
#include <stdio.h>


#ifndef DBGB_LEVEL
#define DBGB_LEVEL 5
#endif

#if DBGB_LEVEL > 4
#define LOGD(format,...) printf("<D>" ":"format"\n", ##__VA_ARGS__)
#else
#define LOGD(...) do{}while(0)
#endif

#if DBGB_LEVEL > 3
#define LOGI(format,...) printf("<I>" ":"format"\n", ##__VA_ARGS__)
#else
#define LOGI(...) do{}while(0)
#endif

#if DBGB_LEVEL > 2
#define LOGW(format,...) printf("[%s:%d]<W>" ":"format"",__FILE__,__LINE__,##__VA_ARGS__)
#else
#define LOGW(...) do{}while(0)
#endif


#if DBGB_LEVEL > 1
#define LOGE(format,...) printf("[%s:%d]<E>" ":"format"\n",__FILE__,__LINE__,##__VA_ARGS__)
#else
#define LOGE(...) do{}while(0)
#endif




#define LOGT      "Talk-TOP    | "
#ifndef DBGT_LEVEL
#define DBGT_LEVEL 4
#endif

#if DBGT_LEVEL > 3
#define LOGD_T(format,...) printf(LOGT"Debug : "format"", ##__VA_ARGS__)
#else
#define LOGD_T(...) do{}while(0)
#endif

#if DBGT_LEVEL > 2
#define LOGI_T(format,...) printf(LOGT"Info  : "format"", ##__VA_ARGS__)
#else
#define LOGI_T(...) do{}while(0)
#endif


#if DBGT_LEVEL > 1
#define LOGE_T(format,...) printf(LOGT"Err   : "format"", ##__VA_ARGS__) // 定义LOGE类型
#else
#define LOGE_T(...) do{}while(0)
#endif

























#endif




