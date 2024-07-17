#include <xmf/xmf.h>
#ifndef XMF_MUXER_H
#define XMF_MUXER_H

typedef struct xmf_webm_muxer XmfWebMMuxer;
#ifdef __cplusplus
extern "C" {
#endif

	XMF_EXPORT XmfWebMMuxer* XmfWebMMuxer_New();

	XMF_EXPORT int XmfWebMMuxer_Remux(XmfWebMMuxer* muxer, const char* inputPath, const char* outputPath);

	XMF_EXPORT void XmfWebMMuxer_Free(XmfWebMMuxer *muxer);

#ifdef __cplusplus
}
#endif

#endif /* XMF_WEBMMUXER_H */
