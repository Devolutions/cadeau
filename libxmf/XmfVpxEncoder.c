#include "XmfVpxEncoder.h"
#include <stdlib.h>
#include <string.h>
#include <vpx/vpx_encoder.h>
#include <vpx/vp8cx.h>

struct xmf_vpx_encoder
{
	vpx_codec_ctx_t codec;
	vpx_codec_enc_cfg_t cfg;
	vpx_codec_pts_t pts;
	XmfVpxEncoderError lastError;
};

XmfVpxEncoder *XmfVpxEncoder_Create(XmfVpxEncoderConfig config)
{
	XmfVpxEncoder *encoder = (XmfVpxEncoder *)malloc(sizeof(XmfVpxEncoder));
	if (!encoder)
	{
		return NULL;
	}

	vpx_codec_iface_t *iface = NULL;
	if (config.codec == VP8)
	{
		iface = vpx_codec_vp8_cx();
	}
	else if (config.codec == VP9)
	{
		iface = vpx_codec_vp9_cx();
	}
	else
	{
		free(encoder);
		return NULL; // Invalid codec parameter
	}

	vpx_codec_err_t res = vpx_codec_enc_config_default(iface, &encoder->cfg, 0);
	if (res != VPX_CODEC_OK)
	{
		free(encoder);
		return NULL; // Failed to get default config
	}

	// Set encoding parameters
	encoder->cfg.g_w = config.width;
	encoder->cfg.g_h = config.height;
	encoder->cfg.rc_target_bitrate = config.bitrate;
	encoder->cfg.g_timebase.num = config.timebase_den;
	encoder->cfg.g_timebase.den = config.timebase_num;
	encoder->cfg.g_threads = config.threads;
	encoder->cfg.g_error_resilient = 1;
	
	// allow keyframes at any time
	encoder->cfg.kf_mode = VPX_KF_AUTO;
	encoder->cfg.kf_min_dist = 0;
	encoder->cfg.kf_max_dist = 9999;

	// Initialize codec
	res = vpx_codec_enc_init(&encoder->codec, iface, &encoder->cfg, 0);
	if (res != VPX_CODEC_OK)
	{
		free(encoder);
		return NULL; // Failed to initialize codec
	}

	encoder->pts = 0;
	encoder->lastError.code = NO_ERROR;

	return encoder;
}

int XmfVpxEncoder_EncodeFrame(XmfVpxEncoder *encoder, const XmfVpxImage *image, int64_t pts, int64_t duration, unsigned int flags)
{
	if (!encoder || !image)
	{
		return -1;
	}

	vpx_image_t *vpx_image = XmfVpxImage_GetData(image);

	if (!vpx_image)
	{
		encoder->lastError.code = INVALID_PARAM;
		return -1;
	}

	vpx_codec_err_t res = vpx_codec_encode(&encoder->codec, vpx_image, pts, duration, flags, VPX_DL_REALTIME);
	if (res != VPX_CODEC_OK)
	{
		encoder->lastError.code = VPX_ERROR;
		encoder->lastError.detail.vpx_error.error_code = res;
		return -1;
	}
	// Reset the iterator after encoding
	return 0;
}

int XmfVpxEncoder_GetEncodedFrame(XmfVpxEncoder *encoder, uint8_t **output, size_t *output_size)
{

	// Retrieve the encoded data
	const vpx_codec_cx_pkt_t *pkt;
	vpx_codec_iter_t itr = NULL;
	while ((pkt = vpx_codec_get_cx_data(&encoder->codec, &itr)) != NULL)
	{
		if (pkt->kind == VPX_CODEC_CX_FRAME_PKT)
		{
			*output_size = pkt->data.frame.sz;
			*output = (uint8_t *)malloc(*output_size);
			if (!*output)
			{
				encoder->lastError.code = MEMORY_ERROR;
				return -1;
			}
			memcpy(*output, pkt->data.frame.buf, *output_size);
			encoder->lastError.code = NO_ERROR;
			return 0;
		}
	}

	// No frame was produced
	*output = NULL;
	*output_size = 0;
	encoder->lastError.code = NO_ERROR;
	return 0;
}

void XmfVpxEncoder_FreeEncodedFrame(uint8_t *output)
{
	if (output)
	{
		free(output);
	}
}

int XmfVpxEncoder_Flush(XmfVpxEncoder *encoder)
{
	if (!encoder)
	{
		return -1;
	}

	vpx_codec_err_t res = vpx_codec_encode(&encoder->codec, NULL, encoder->pts, 1, 0, VPX_DL_REALTIME);
	if (res != VPX_CODEC_OK)
	{
		encoder->lastError.code = VPX_ERROR;
		encoder->lastError.detail.vpx_error.error_code = res;
		return -1;
	}
	// Reset the iterator after flushing
	encoder->lastError.code = NO_ERROR;
	return 0;
}

int XmfVpxEncoder_Destroy(XmfVpxEncoder *encoder)
{
	if (encoder)
	{
		vpx_codec_destroy(&encoder->codec);
		free(encoder);
	}
	return 0;
}

XmfVpxEncoderError XmfVpxEncoder_GetLastError(const XmfVpxEncoder *encoder)
{
	if (encoder)
	{
		return encoder->lastError;
	}
	else
	{
		XmfVpxEncoderError err = {INVALID_PARAM};
		return err;
	}
}
