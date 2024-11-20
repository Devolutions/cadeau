

#include "XmfVpxFrame.h"
#include <stdlib.h>
#include <string.h>

struct xmf_vpx_frame
{
    void *buf;                                        /**< compressed data buffer */
    size_t sz;                                        /**< length of compressed data */
    vpx_codec_pts_t pts;                              /**< time stamp to show frame (in timebase units) */
    unsigned long duration;                           /**< duration to show frame (in timebase units) */
    vpx_codec_frame_flags_t flags;                    /**< flags for this frame */
    int partition_id;                                 /**< partition id defines the decoding order of the partitions */
    unsigned int width[VPX_SS_MAX_LAYERS];            /**< frame width */
    unsigned int height[VPX_SS_MAX_LAYERS];           /**< frame height */
    uint8_t spatial_layer_encoded[VPX_SS_MAX_LAYERS]; /**< flag to indicate if spatial layer frame is encoded or dropped */
};

// Deep copy of the frame data
XmfVpxFrame *XmfVpxFrame_Create(const vpx_codec_cx_pkt_t *data)
{
    if (!data || data->kind != VPX_CODEC_CX_FRAME_PKT)
    {
        return NULL;
    }

    XmfVpxFrame *frame = (XmfVpxFrame *)malloc(sizeof(XmfVpxFrame));
    if (!frame)
    {
        return NULL;
    }

    frame->sz = data->data.frame.sz;
    frame->buf = malloc(frame->sz);
    if (!frame->buf)
    {
        free(frame);
        return NULL;
    }
    memcpy(frame->buf, data->data.frame.buf, frame->sz);

    frame->pts = data->data.frame.pts;
    frame->duration = data->data.frame.duration;
    frame->flags = data->data.frame.flags;
    frame->partition_id = data->data.frame.partition_id;
    memcpy(frame->width, data->data.frame.width, sizeof(frame->width));
    memcpy(frame->height, data->data.frame.height, sizeof(frame->height));
    memcpy(frame->spatial_layer_encoded, data->data.frame.spatial_layer_encoded, sizeof(frame->spatial_layer_encoded));

    return frame;
}

void XmfVpxFrame_Destroy(XmfVpxFrame *frame)
{
    if (frame)
    {
        if (frame->buf)
        {
            free(frame->buf);
        }
        free(frame);
    }
}

size_t XmfVpxFrame_GetSize(const XmfVpxFrame *frame)
{
    return frame->sz;
}

vpx_codec_pts_t XmfVpxFrame_GetPts(const XmfVpxFrame *frame)
{
    return frame->pts;
}

unsigned long XmfVpxFrame_GetDuration(const XmfVpxFrame *frame)
{
    return frame->duration;
}

vpx_codec_frame_flags_t XmfVpxFrame_GetFlags(const XmfVpxFrame *frame)
{
    return frame->flags;
}

int XmfVpxFrame_GetPartitionId(const XmfVpxFrame *frame)
{
    return frame->partition_id;
}

unsigned int XmfVpxFrame_GetWidth(const XmfVpxFrame *frame, int layer)
{
    if (layer < 0 || layer >= VPX_SS_MAX_LAYERS)
    {
        return 0; // or handle error appropriately
    }
    return frame->width[layer];
}

unsigned int XmfVpxFrame_GetHeight(const XmfVpxFrame *frame, int layer)
{
    if (layer < 0 || layer >= VPX_SS_MAX_LAYERS)
    {
        return 0; // or handle error appropriately
    }
    return frame->height[layer];
}

uint8_t XmfVpxFrame_GetSpatialLayerEncoded(const XmfVpxFrame *frame, int layer)
{
    if (layer < 0 || layer >= VPX_SS_MAX_LAYERS)
    {
        return 0; // or handle error appropriately
    }
    return frame->spatial_layer_encoded[layer];
}

int XmfVpxFrame_GetBuffer(const XmfVpxFrame *frame, const uint8_t **buffer, size_t *size)
{
    if (!frame || !buffer || !size)
    {
        return -1;
    }

    *buffer = malloc(frame->sz);

    if (!*buffer)
    {
        return -1;
    }

    memcpy((void *)*buffer, frame->buf, frame->sz);

    *size = frame->sz;

    return 0;
}