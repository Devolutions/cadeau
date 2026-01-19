#include "XmfVpxEncoder.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vpx/vpx_encoder.h>
#include <vpx/vp8cx.h>
#include "XmfVpxPacket.h"

#ifndef VP9E_CONTENT_SCREEN
#define VP9E_CONTENT_SCREEN 1
#endif

struct xmf_vpx_encoder
{
	vpx_codec_ctx_t codec;
	vpx_codec_enc_cfg_t cfg;
	vpx_codec_pts_t pts;
	XmfVpxEncoderError lastError;
	XmfVpxCodecType codec_type;
};

/*
 * XMF libvpx realtime presets.
 *
 * Background:
 * - Devolutions Gateway uses XMF to re-encode WebM blocks when cutting into an uncued stream.
 * - The worst-case workload is: decode a run of inter frames up to the cut, then force the cut frame
 *   to become a keyframe, and (in some modes) force many subsequent frames as keyframes as well.
 * - In this workload, libvpx "lookahead / lagged encoding" (g_lag_in_frames) is disastrous for latency
 *   and can cause seconds-per-frame behavior.
 *
 * Goal:
 * - Provide explicit, stable presets via parameters (not environment variables).
 * - Keep defaults sane and stable for realtime streaming.
 *
 * NOTE ABOUT VP9 ROW-MT:
 * - We observed reproducible process crashes (heap corruption / access violation) when using VP9
 *   row-multithreading (VP9E_SET_ROW_MT=1) together with g_threads>1 on our Windows libvpx build.
 * - Therefore, for stability, we hard-disable row-mt whenever threads>1, regardless of preset.
 */

static int xmf_vpx_apply_control(XmfVpxEncoder *encoder, int ctrl_id, int value)
{
	vpx_codec_err_t res = vpx_codec_control_(&encoder->codec, ctrl_id, value);
	if (res != VPX_CODEC_OK)
	{
		encoder->lastError.code = VPX_ERROR;
		encoder->lastError.detail.vpx_error.error_code = res;
		return -1;
	}

	return 0;
}

static uint32_t xmf_u32_clamp(uint32_t value, uint32_t min, uint32_t max)
{
	if (value < min)
	{
		return min;
	}
	if (value > max)
	{
		return max;
	}
	return value;
}

static void xmf_log_effective_vpx_settings_if_enabled(
	const XmfVpxEncoder *encoder,
	int cpuused_value,
	int screen_content_mode_value,
	int enable_auto_altref_value,
	int dropframe_thresh_value,
	int lag_in_frames_value,
	int threads_value,
	int arnr_maxframes_value,
	int arnr_strength_value,
	int arnr_type_value,
	int row_mt_value,
	int tile_columns_log2_value,
	int tile_rows_log2_value,
	int tune_content_value,
	int frame_parallel_decoding_value)
{
	if (encoder->cfg.g_w == 0 || encoder->cfg.g_h == 0)
	{
		/* Nothing useful to log. */
		return;
	}

	fprintf(
		stderr,
		"[LibVPx-Performance-Hypothesis][XMF] effective cfg codec=%s w=%u h=%u threads=%u bitrate=%u timebase=%u/%u dropframe_thresh=%u lag_in_frames=%u error_resilient=%u kf_mode=%d kf_min=%u kf_max=%u\n",
		encoder->codec_type == VP8 ? "VP8" : (encoder->codec_type == VP9 ? "VP9" : "UNKNOWN"),
		encoder->cfg.g_w,
		encoder->cfg.g_h,
		encoder->cfg.g_threads,
		encoder->cfg.rc_target_bitrate,
		encoder->cfg.g_timebase.num,
		encoder->cfg.g_timebase.den,
		encoder->cfg.rc_dropframe_thresh,
		encoder->cfg.g_lag_in_frames,
		encoder->cfg.g_error_resilient,
		encoder->cfg.kf_mode,
		encoder->cfg.kf_min_dist,
		encoder->cfg.kf_max_dist);

	fprintf(stderr, "[LibVPx-Performance-Hypothesis][XMF] preset cpuused=%d\n", cpuused_value);
	fprintf(stderr, "[LibVPx-Performance-Hypothesis][XMF] preset threads=%d\n", threads_value);
	fprintf(stderr, "[LibVPx-Performance-Hypothesis][XMF] preset dropframe_thresh=%d\n", dropframe_thresh_value);
	fprintf(stderr, "[LibVPx-Performance-Hypothesis][XMF] preset lag_in_frames=%d\n", lag_in_frames_value);
	fprintf(stderr, "[LibVPx-Performance-Hypothesis][XMF] preset enable_auto_altref=%d\n", enable_auto_altref_value);
	fprintf(stderr, "[LibVPx-Performance-Hypothesis][XMF] preset arnr_maxframes=%d\n", arnr_maxframes_value);
	fprintf(stderr, "[LibVPx-Performance-Hypothesis][XMF] preset arnr_strength=%d\n", arnr_strength_value);
	fprintf(stderr, "[LibVPx-Performance-Hypothesis][XMF] preset arnr_type=%d\n", arnr_type_value);

	if (encoder->codec_type == VP8)
	{
		fprintf(stderr, "[LibVPx-Performance-Hypothesis][XMF] preset vp8_screen_content_mode=%d\n", screen_content_mode_value);
	}

	if (encoder->codec_type == VP9)
	{
		fprintf(stderr, "[LibVPx-Performance-Hypothesis][XMF] preset vp9_row_mt=%d\n", row_mt_value);
		fprintf(stderr, "[LibVPx-Performance-Hypothesis][XMF] preset vp9_tile_columns_log2=%d\n", tile_columns_log2_value);
		fprintf(stderr, "[LibVPx-Performance-Hypothesis][XMF] preset vp9_tile_rows_log2=%d\n", tile_rows_log2_value);
		fprintf(stderr, "[LibVPx-Performance-Hypothesis][XMF] preset vp9_tune_content=%d\n", tune_content_value);
		fprintf(stderr, "[LibVPx-Performance-Hypothesis][XMF] preset vp9_frame_parallel_decoding=%d\n", frame_parallel_decoding_value);
	}

	fflush(stderr);
}

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
	encoder->cfg.g_timebase.num = config.timebase_num;
	encoder->cfg.g_timebase.den = config.timebase_den;
	encoder->cfg.g_error_resilient = 1;

	int cpuused_value = 0;
	int screen_content_mode_value = 0;
	int enable_auto_altref_value = 0;
	int dropframe_thresh_value = 0;
	int lag_in_frames_value = 0;
	int threads_value = 0;
	int arnr_maxframes_value = 0;
	int arnr_strength_value = 0;
	int arnr_type_value = 0;
	int row_mt_value = 0;
	int tile_columns_log2_value = 0;
	int tile_rows_log2_value = 0;
	int tune_content_value = 0;
	int frame_parallel_decoding_value = 0;

	/*
	 * Threading:
	 * - The caller provides a desired thread count via config.threads.
	 * - We clamp it to avoid invalid/unstable values observed in production (e.g. threads=20 causing init failures).
	 * - IMPORTANT: VP9 row-mt is disabled for threads>1 due to crashes on our libvpx build.
	 */
	threads_value = (int)xmf_u32_clamp(config.threads, 1, 8);
	encoder->cfg.g_threads = (unsigned int)threads_value;

	/*
	 * Realtime defaults:
	 * - g_lag_in_frames must be 0 to disable lookahead. This is the single most important knob for our workload.
	 * - rc_dropframe_thresh lets the encoder drop frames under pressure rather than accumulating latency.
	 * - enable_auto_alt_ref and ARNR are quality features that add work/latency; we disable them.
	 */
	lag_in_frames_value = 0;
	encoder->cfg.g_lag_in_frames = (unsigned int)lag_in_frames_value;

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
	encoder->codec_type = config.codec;

	/*
	 * Presets:
	 * - DEFAULT: conservative realtime defaults.
	 * - SANE: realtime oriented, but not the most aggressive.
	 * - BEST_PERFORMANCE: maximum throughput, quality is not a goal.
	 */
	switch (config.preset)
	{
	case XMF_VPX_PRESET_DEFAULT:
	default:
		dropframe_thresh_value = 0;
		cpuused_value = encoder->codec_type == VP9 ? 4 : 6;
		tile_columns_log2_value = 0;
		tile_rows_log2_value = 0;
		tune_content_value = 0;
		screen_content_mode_value = 1;
		break;
	case XMF_VPX_PRESET_SANE:
		dropframe_thresh_value = 5;
		cpuused_value = encoder->codec_type == VP9 ? 6 : 8;
		tile_columns_log2_value = encoder->codec_type == VP9 ? 1 : 0;
		tile_rows_log2_value = 0;
		tune_content_value = encoder->codec_type == VP9 ? VP9E_CONTENT_SCREEN : 0;
		screen_content_mode_value = 1;
		break;
	case XMF_VPX_PRESET_BEST_PERFORMANCE:
		dropframe_thresh_value = 10;
		cpuused_value = encoder->codec_type == VP9 ? 9 : 12;
		tile_columns_log2_value = encoder->codec_type == VP9 ? 2 : 0;
		tile_rows_log2_value = 0;
		tune_content_value = encoder->codec_type == VP9 ? VP9E_CONTENT_SCREEN : 0;
		screen_content_mode_value = 1;
		break;
	}

	encoder->cfg.rc_dropframe_thresh = (unsigned int)dropframe_thresh_value;

	// Note: VP8E_SET_CPUUSED is used for both VP8 and VP9 in libvpx.
	if (xmf_vpx_apply_control(encoder, VP8E_SET_CPUUSED, cpuused_value) != 0)
	{
		XmfVpxEncoder_Destroy(encoder);
		return NULL;
	}

	// enable_auto_alt_ref enables alternative reference frames.
	// This can improve quality but adds complexity/latency, which we do not want in realtime streaming.
	// Default is disabled for all presets.
	enable_auto_altref_value = 0;
	if (xmf_vpx_apply_control(encoder, VP8E_SET_ENABLEAUTOALTREF, enable_auto_altref_value) != 0)
	{
		XmfVpxEncoder_Destroy(encoder);
		return NULL;
	}

	if (encoder->codec_type == VP8)
	{
		// VP8 screen content mode tunes the encoder for UI/desktop content.
		if (xmf_vpx_apply_control(encoder, VP8E_SET_SCREEN_CONTENT_MODE, screen_content_mode_value) != 0)
		{
			XmfVpxEncoder_Destroy(encoder);
			return NULL;
		}

		// ARNR is a VP8-centric temporal filtering knob.
		// Keep it disabled for realtime performance.
		arnr_maxframes_value = 0;
		arnr_strength_value = 0;
		arnr_type_value = 0;

		if (xmf_vpx_apply_control(encoder, VP8E_SET_ARNR_MAXFRAMES, arnr_maxframes_value) != 0)
		{
			XmfVpxEncoder_Destroy(encoder);
			return NULL;
		}
		if (xmf_vpx_apply_control(encoder, VP8E_SET_ARNR_STRENGTH, arnr_strength_value) != 0)
		{
			XmfVpxEncoder_Destroy(encoder);
			return NULL;
		}
		if (xmf_vpx_apply_control(encoder, VP8E_SET_ARNR_TYPE, arnr_type_value) != 0)
		{
			XmfVpxEncoder_Destroy(encoder);
			return NULL;
		}
	}

	if (encoder->codec_type == VP9)
	{
		// VP9 row-multithreading (row-mt) is forced OFF for stability (see note above).
		row_mt_value = 0;
		if (xmf_vpx_apply_control(encoder, VP9E_SET_ROW_MT, row_mt_value) != 0)
		{
			XmfVpxEncoder_Destroy(encoder);
			return NULL;
		}

		if (xmf_vpx_apply_control(encoder, VP9E_SET_TILE_COLUMNS, tile_columns_log2_value) != 0)
		{
			XmfVpxEncoder_Destroy(encoder);
			return NULL;
		}

		if (xmf_vpx_apply_control(encoder, VP9E_SET_TILE_ROWS, tile_rows_log2_value) != 0)
		{
			XmfVpxEncoder_Destroy(encoder);
			return NULL;
		}

		if (xmf_vpx_apply_control(encoder, VP9E_SET_TUNE_CONTENT, tune_content_value) != 0)
		{
			XmfVpxEncoder_Destroy(encoder);
			return NULL;
		}

#ifdef VP9E_SET_FRAME_PARALLEL_DECODING
		// Keep frame-parallel decoding enabled for better decoder parallelism.
		frame_parallel_decoding_value = 1;
		if (xmf_vpx_apply_control(encoder, VP9E_SET_FRAME_PARALLEL_DECODING, frame_parallel_decoding_value) != 0)
		{
			XmfVpxEncoder_Destroy(encoder);
			return NULL;
		}
#endif
	}

	if (config.log_effective != 0)
	{
		xmf_log_effective_vpx_settings_if_enabled(
			encoder,
			cpuused_value,
			screen_content_mode_value,
			enable_auto_altref_value,
			dropframe_thresh_value,
			lag_in_frames_value,
			threads_value,
			arnr_maxframes_value,
			arnr_strength_value,
			arnr_type_value,
			row_mt_value,
			tile_columns_log2_value,
			tile_rows_log2_value,
			tune_content_value,
			frame_parallel_decoding_value);
	}

	return encoder;
}

int XmfVpxEncoder_EncodeFrame(XmfVpxEncoder *encoder, const XmfVpxImage *image, vpx_codec_pts_t pts, unsigned long duration, unsigned int flags)
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

	return 0;
}

int XmfVpxEncoder_GetEncodedFrame(XmfVpxEncoder *encoder, uint8_t **output, size_t *output_size)
{

	if (!encoder || !output || !output_size)
	{
		return -1;
	}

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

XmfVpxPacket *XmfVpxEncoder_GetPacket(XmfVpxEncoder *encoder, vpx_codec_iter_t *iter)
{
	const vpx_codec_cx_pkt_t *pkt;

	pkt = vpx_codec_get_cx_data(&encoder->codec, iter);
	if (pkt == NULL)
	{
		return NULL;
	}

	XmfVpxPacket *packet = XmfVpxPacket_Create(pkt);

	return packet;
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
