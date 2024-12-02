#include "XmfVpxDecoder.h"
#include <stdlib.h>
#include <string.h>
#include <vpx/vp8dx.h>
#include <vpx/vpx_decoder.h>
#include "XmfVpxImage.h"

// Internal decoder structure
struct xmf_vpx_decoder
{
    vpx_codec_ctx_t codec;
    XmfVpxDecoderError lastError;
};

XmfVpXDecoder *XmfVpxDecoder_Create(XmfVpxDecoderConfig cfg)
{
    // uncomment this to trigger the memory error, this is for sanity check with memory sanitizer
    // int* ptr = NULL;
    // printf("hello %d", *ptr);

    XmfVpXDecoder *decoder = (XmfVpXDecoder *)calloc(1, sizeof(XmfVpXDecoder));
    if (!decoder)
    {
        return NULL;
    }

    decoder->lastError.code = NO_ERROR;

    vpx_codec_dec_cfg_t dec_cfg = {
        .threads = cfg.threads,
        .w = cfg.w,
        .h = cfg.h};

    vpx_codec_iface_t *iface = (cfg.codec == VP8) ? vpx_codec_vp8_dx() : vpx_codec_vp9_dx();

    vpx_codec_err_t res = vpx_codec_dec_init(&decoder->codec, iface, &dec_cfg, 0);
    if (res != VPX_CODEC_OK)
    {
        free(decoder);
        return NULL;
    }

    return decoder;
}

int XmfVpxDecoder_Decode(XmfVpXDecoder *decoder, const uint8_t *data, unsigned int size)
{
    if (!decoder || !data || size == 0)
    {
        return -1;
    }

    vpx_codec_err_t res = vpx_codec_decode(&decoder->codec, data, size, NULL, 0);
    if (res != VPX_CODEC_OK)
    {
        decoder->lastError.code = VPX_ERROR;
        decoder->lastError.detail.vpx_error.error_code = res;
        return -1;
    }

    decoder->lastError.code = NO_ERROR;
    return 0;
}

XmfVpxImage *XmfVpxDecoder_GetNextFrame(XmfVpXDecoder *decoder)
{
    if (!decoder)
    {
        return NULL;
    }

    vpx_codec_iter_t itr = NULL;
    vpx_image_t *image = vpx_codec_get_frame(&decoder->codec, &itr);
    if (!image)
    {
        decoder->lastError.code = NO_FRAME_AVAILABLE;
        return NULL;
    }

    decoder->lastError.code = NO_ERROR;
    XmfVpxImage *result = XmfVpxImage_Create(image);
    return result;
}

XmfVpxDecoderError XmfVpxDecoder_GetLastError(XmfVpXDecoder *decoder)
{
    XmfVpxDecoderError error = {0};
    if (decoder)
    {
        error = decoder->lastError;
    }
    else
    {
        error.code = MEMORY_ERROR;
    }
    return error;
}

void XmfVpxDecoder_Destroy(XmfVpXDecoder *decoder)
{
    if (decoder)
    {
        vpx_codec_destroy(&decoder->codec);
        free(decoder);
    }
}
