#ifndef XMF_WEBM_H
#define XMF_WEBM_H

#include <xmf/xmf.h>

#define VP8_FOURCC 0x30385056
#define VP9_FOURCC 0x30395056

typedef struct xmf_webm XmfWebM;

#include "XmfTime.h"
#include "XmfBipBuffer.h"

#ifdef __cplusplus
extern "C"
{
#endif

    XMF_EXPORT int XmfWebM_Encode(XmfWebM *ctx, const uint8_t *srcData, uint16_t x, uint16_t y, uint16_t width, uint16_t height);
    XMF_EXPORT void XmfWebM_Finalize(XmfWebM *ctx);

    XMF_EXPORT uint64_t XmfWebM_FrameCount(XmfWebM *ctx);
    XMF_EXPORT uint64_t XmfWebM_Duration(XmfWebM *ctx);

    XMF_EXPORT bool XmfWebM_Init(XmfWebM *ctx, uint32_t frameWidth, uint32_t frameHeight, uint32_t frameRate,
                                 uint32_t targetBitRate, const char *filename, XmfBipBuffer *bb, XmfTimeSource *ts);

    XMF_EXPORT void XmfWebM_Uninit(XmfWebM *ctx);

    XMF_EXPORT XmfWebM *XmfWebM_New();
    XMF_EXPORT void XmfWebM_Free(XmfWebM *ctx);

#ifdef __cplusplus
}
#endif

#endif /* XMF_WEBM_H */
