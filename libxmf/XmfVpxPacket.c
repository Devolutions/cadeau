
#include "XmfVpxPacket.h"
#include <vpx/vpx_encoder.h>

struct xmf_vpx_packet
{
    const vpx_codec_cx_pkt_t *pkt;
};

XmfVpxPacket *XmfVpxPacket_Create(const vpx_codec_cx_pkt_t *data)
{

    if (!data)
    {
        return NULL;
    }

    XmfVpxPacket *packet = (XmfVpxPacket *)malloc(sizeof(XmfVpxPacket));

    if (!packet)
    {
        return NULL;
    }

    packet->pkt = data;
    return packet;
}

void XmfVpxPacket_Destroy(XmfVpxPacket *packet)
{
    // the pointer is not heap allocated
    // the pointer may only be valid until the next call to vpx_codec_*
    if (packet)
    {
        free(packet);
    }
}

int XmfVpxPacket_SetData(XmfVpxPacket *packet, vpx_codec_cx_pkt_t *data)
{
    if (!packet)
    {
        return -1;
    }

    packet->pkt = data;
    return 1;
}

const vpx_codec_cx_pkt_t *XmfVpxPacket_GetData(const XmfVpxPacket *packet)
{
    if (!packet)
    {
        return NULL;
    }

    return packet->pkt;
}

XmfVpxPacketKind XmfVpxPacket_GetKind(const XmfVpxPacket *packet)
{
    if (!packet)
    {
        return -1;
    }

    switch (packet->pkt->kind)
    {
    case VPX_CODEC_CX_FRAME_PKT:
        return XMF_VPX_CODEC_CX_FRAME_PKT;
    case VPX_CODEC_STATS_PKT:
        return XMF_VPX_CODEC_STATS_PKT;
    case VPX_CODEC_FPMB_STATS_PKT:
        return XMF_VPX_CODEC_FPMB_STATS_PKT;
    case VPX_CODEC_PSNR_PKT:
        return XMF_VPX_CODEC_PSNR_PKT;
    case VPX_CODEC_CUSTOM_PKT:
        return XMF_VPX_CODEC_CUSTOM_PKT;
    default:
        return -1;
    }
}

XmfVpxFrame *XmfVpxPacket_GetFrame(const XmfVpxPacket *packet)
{

    if (!packet || packet->pkt->kind != VPX_CODEC_CX_FRAME_PKT)
    {
        return NULL;
    }

    return XmfVpxFrame_Create(packet->pkt);
}

bool XmfVpxPacket_IsEmpty(const XmfVpxPacket *packet) {
    return packet == NULL || packet->pkt == NULL;
}