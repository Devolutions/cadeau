#ifndef XMF_RECORDER_H
#define XMF_RECORDER_H

#include <xmf/xmf.h>

#define XMF_RECORDER_QUALITY_MIN 1
#define XMF_RECORDER_QUALITY_MAX 10

#define XMF_RECORDER_FRAME_RATE_MIN 1
#define XMF_RECORDER_FRAME_RATE_MAX 30

typedef struct now_recorder XmfRecorder;

#ifdef __cplusplus
extern "C" {
#endif

XMF_EXPORT uint32_t XmfRecorder_CalculateBitRate(uint32_t frameWidth, uint32_t frameHeight, uint32_t frameRate, uint32_t quality);

XMF_EXPORT int XmfRecorder_Update(XmfRecorder* ctx, uint8_t* frameData, uint32_t frameStep,
				   uint32_t frameWidth, uint32_t frameHeight, uint32_t updateX,
				   uint32_t updateY, uint32_t updateWidth, uint32_t updateHeight);
XMF_EXPORT void XMF_API XmfRecorder_UpdateFrame(XmfRecorder* ctx, uint8_t* buffer, uint32_t updateX, uint32_t updateY,
                                 uint32_t updateWidth, uint32_t updateHeight, uint32_t surfaceStep);
XMF_EXPORT uint32_t XmfRecorder_GetTimeout(XmfRecorder* ctx);
XMF_EXPORT void XmfRecorder_Timeout(XmfRecorder* ctx);

XMF_EXPORT void XmfRecorder_SetMinimumFrameRate(XmfRecorder* ctx, uint32_t frameRate);
XMF_EXPORT void XmfRecorder_SetFrameRate(XmfRecorder* ctx, uint32_t frameRate);
XMF_EXPORT void XmfRecorder_SetFrameSize(XmfRecorder* ctx, uint32_t frameWidth, uint32_t frameHeight);
XMF_EXPORT void XmfRecorder_SetVideoQuality(XmfRecorder* ctx, uint32_t videoQuality);

XMF_EXPORT void XmfRecorder_SetFilename(XmfRecorder* ctx, const char* filename);
XMF_EXPORT void XmfRecorder_SetDirectory(XmfRecorder* ctx, const char* directory);
XMF_EXPORT size_t XmfRecorder_GetPath(XmfRecorder* ctx, char* path, size_t size);

XMF_EXPORT bool XmfRecorder_IsEnabled(XmfRecorder* ctx);
XMF_EXPORT void XmfRecorder_SetEnabled(XmfRecorder* ctx, bool enabled);

XMF_EXPORT bool XmfRecorder_Init(XmfRecorder* ctx);
XMF_EXPORT void XmfRecorder_Uninit(XmfRecorder* ctx);

XMF_EXPORT XmfRecorder* XmfRecorder_New();
XMF_EXPORT void XmfRecorder_Free(XmfRecorder* ctx);

#ifdef __cplusplus
}
#endif

#endif /* XMF_RECORDER_H */
