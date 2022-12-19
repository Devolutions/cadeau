#ifndef XMF_WEBM_H
#define XMF_WEBM_H

#include <xmf/xmf.h>

#define VP8_FOURCC 0x30385056
#define VP9_FOURCC 0x30395056

typedef uint64_t now_webm_time;
typedef struct now_webm XmfWebM;

#ifdef __cplusplus
extern "C" {
#endif

int XmfWebM_Encode(XmfWebM* ctx, uint8_t* srcData, uint16_t x, uint16_t y, uint16_t width, uint16_t height);
void XmfWebM_Finalize(XmfWebM* ctx);

uint64_t XmfWebM_FrameCount(XmfWebM* ctx);
now_webm_time XmfWebM_Duration(XmfWebM* ctx);

bool XmfWebM_Init(XmfWebM* ctx, uint32_t frameWidth, uint32_t frameHeight, uint32_t frameRate,
                uint32_t targetBitRate, const char* filename);
void XmfWebM_Uninit(XmfWebM* ctx);

XmfWebM* XmfWebM_New();
void XmfWebM_Free(XmfWebM* ctx);

#ifdef __cplusplus
}
#endif

#endif /* XMF_WEBM_H */
