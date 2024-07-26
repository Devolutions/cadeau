#ifndef XMF_TIME_H
#define XMF_TIME_H

#include <xmf/xmf.h>

#ifdef __cplusplus
extern "C" {
#endif

XMF_EXPORT typedef uint64_t (*XMF_GET_TIME_FN)(void* param);

XMF_EXPORT typedef struct
{
    XMF_GET_TIME_FN func;
    void* param;
} XmfTimeSource;

XMF_EXPORT uint64_t XmfTime_GetTickCount();

XMF_EXPORT uint64_t XmfTimeSource_Get(XmfTimeSource* ts);

XMF_EXPORT uint64_t XmfTimeSource_System(void* param);

XMF_EXPORT uint64_t XmfTimeSource_Manual(void* param);

#ifdef __cplusplus
}
#endif

#endif /* XMF_TIME_H */
