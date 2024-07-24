#ifndef XMF_RECORDER_H
#define XMF_RECORDER_H

#include <xmf/xmf.h>

#define XMF_RECORDER_QUALITY_MIN 1
#define XMF_RECORDER_QUALITY_MAX 10

#define XMF_RECORDER_FRAME_RATE_MIN 1
#define XMF_RECORDER_FRAME_RATE_MAX 30

typedef struct xmf_recorder XmfRecorder;

#include "XmfBipBuffer.h"

#ifdef __cplusplus
extern "C" {
#endif

XMF_EXPORT uint32_t XmfRecorder_CalculateBitRate(uint32_t frameWidth, uint32_t frameHeight, uint32_t frameRate, uint32_t quality);

XMF_EXPORT void XMF_API XmfRecorder_UpdateFrame(XmfRecorder* ctx, const uint8_t* buffer, uint32_t updateX, uint32_t updateY,
                                 uint32_t updateWidth, uint32_t updateHeight, uint32_t surfaceStep);
XMF_EXPORT uint32_t XmfRecorder_GetTimeout(XmfRecorder* ctx);
XMF_EXPORT void XmfRecorder_Timeout(XmfRecorder* ctx);

XMF_EXPORT void XmfRecorder_SetMinimumFrameRate(XmfRecorder* ctx, uint32_t frameRate);
XMF_EXPORT uint32_t XmfRecorder_GetFrameRate(XmfRecorder* ctx);
XMF_EXPORT void XmfRecorder_SetFrameRate(XmfRecorder* ctx, uint32_t frameRate);
XMF_EXPORT void XmfRecorder_SetFrameSize(XmfRecorder* ctx, uint32_t frameWidth, uint32_t frameHeight);
XMF_EXPORT void XmfRecorder_SetVideoQuality(XmfRecorder* ctx, uint32_t videoQuality);

XMF_EXPORT void XmfRecorder_SetCurrentTime(XmfRecorder* ctx, uint64_t currentTime);
XMF_EXPORT uint64_t XmfRecorder_GetCurrentTime(XmfRecorder* ctx);

XMF_EXPORT void XmfRecorder_SetFileName(XmfRecorder* ctx, const char* filename);

XMF_EXPORT void XmfRecorder_SetBipBuffer(XmfRecorder* ctx, XmfBipBuffer* bb);

XMF_EXPORT bool XmfRecorder_Init(XmfRecorder* ctx);
XMF_EXPORT void XmfRecorder_Uninit(XmfRecorder* ctx);

XMF_EXPORT XmfRecorder* XmfRecorder_New();
XMF_EXPORT void XmfRecorder_Free(XmfRecorder* ctx);

#ifdef __cplusplus
}
#endif

#endif /* XMF_RECORDER_H */
