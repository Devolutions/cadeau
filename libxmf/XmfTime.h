#ifndef XMF_TIME_H
#define XMF_TIME_H

#include <xmf/xmf.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t (*XMF_GET_TIME_FN)(void* param);

typedef struct
{
    XMF_GET_TIME_FN func;
    void* param;
} XmfTimeSource;

uint64_t XmfTime_GetTickCount();

uint64_t XmfTimeSource_Get(XmfTimeSource* ts);

uint64_t XmfTimeSource_System(void* param);

uint64_t XmfTimeSource_Manual(void* param);

#ifdef __cplusplus
}
#endif

#endif /* XMF_TIME_H */
