#ifndef XMF_VPX_PACKET_H
#define XMF_VPX_PACKET_H

#include <vpx/vpx_codec.h>
#include <xmf/xmf.h>
#include <vpx/vpx_encoder.h>
#include "XmfVpxFrame.h"

/// <div rustbindgen opaque></div>
typedef struct xmf_vpx_packet XmfVpxPacket;

typedef enum xmf_vpx_packet_kind
{
    XMF_VPX_CODEC_CX_FRAME_PKT = VPX_CODEC_CX_FRAME_PKT,     /**< Compressed video frame */
    XMF_VPX_CODEC_STATS_PKT = VPX_CODEC_STATS_PKT,           /**< Two-pass statistics for this frame */
    XMF_VPX_CODEC_FPMB_STATS_PKT = VPX_CODEC_FPMB_STATS_PKT, /**< first pass mb statistics for this frame */
    XMF_VPX_CODEC_PSNR_PKT = VPX_CODEC_PSNR_PKT,             /**< PSNR statistics for this frame */
    XMF_VPX_CODEC_CUSTOM_PKT = 256                           /**< Algorithm extensions  */
} XmfVpxPacketKind;

#ifdef __cplusplus
extern "C"
{
#endif

    XMF_EXPORT void XmfVpxPacket_Destroy(XmfVpxPacket *packet);
    XMF_EXPORT XmfVpxPacketKind XmfVpxPacket_GetKind(const XmfVpxPacket *packet);
    /**
     * Retrieves the frame data from the packet.
     * This function will return NULL if the packet is not a frame packet.
     *
     * @param packet Pointer to the packet instance.
     * @return Pointer to the frame data, or NULL if the packet is not a frame packet.
     *
     * @note The returned frame data is deep copied from the packet data and should be freed by the caller.
     */
    XMF_EXPORT XmfVpxFrame *XmfVpxPacket_GetFrame(const XmfVpxPacket *packet);
    XMF_EXPORT bool XmfVpxPacket_IsEmpty(const XmfVpxPacket *packet);

    XmfVpxPacket *XmfVpxPacket_Create(const vpx_codec_cx_pkt_t *data);
    int XmfVpxPacket_SetData(XmfVpxPacket *packet, vpx_codec_cx_pkt_t *data);
    const vpx_codec_cx_pkt_t *XmfVpxPacket_GetData(const XmfVpxPacket *packet);

#ifdef __cplusplus
}
#endif
#endif // XMF_VPX_PACKET_H