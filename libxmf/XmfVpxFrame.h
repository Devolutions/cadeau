#ifndef XMF_VPX_FRAME_H
#define XMF_VPX_FRAME_H

#include <vpx/vpx_codec.h>
#include <xmf/xmf.h>
#include <vpx/vpx_encoder.h>

typedef struct xmf_vpx_frame XmfVpxFrame;

#ifdef __cplusplus
extern "C"
{
#endif

    XMF_EXPORT void XmfVpxFrame_Destroy(XmfVpxFrame *frame);
    XmfVpxFrame *XmfVpxFrame_Create(const vpx_codec_cx_pkt_t *data);

    XMF_EXPORT size_t XmfVpxFrame_GetSize(const XmfVpxFrame *frame);
    XMF_EXPORT vpx_codec_pts_t XmfVpxFrame_GetPts(const XmfVpxFrame *frame);
    XMF_EXPORT unsigned long XmfVpxFrame_GetDuration(const XmfVpxFrame *frame);
    XMF_EXPORT vpx_codec_frame_flags_t XmfVpxFrame_GetFlags(const XmfVpxFrame *frame);
    XMF_EXPORT int XmfVpxFrame_GetPartitionId(const XmfVpxFrame *frame);
    XMF_EXPORT unsigned int XmfVpxFrame_GetWidth(const XmfVpxFrame *frame, int layer);
    XMF_EXPORT unsigned int XmfVpxFrame_GetHeight(const XmfVpxFrame *frame, int layer);
    XMF_EXPORT uint8_t XmfVpxFrame_GetSpatialLayerEncoded(const XmfVpxFrame *frame, int layer);
    XMF_EXPORT int XmfVpxFrame_GetBuffer(const XmfVpxFrame *frame, const uint8_t **buffer, size_t *size);

#ifdef __cplusplus
}
#endif
#endif // XMF_VPX_FRAME_H