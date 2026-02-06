#include "XmfVpxEncoder.h"
#include <stdlib.h>
#include <string.h>
#include <vpx/vpx_encoder.h>
#include <vpx/vp8cx.h>
#include "XmfVpxPacket.h"

#ifndef VP9E_CONTENT_SCREEN
// Doc: VP9E_SET_TUNE_CONTENT values (SCREEN = 1)
// https://chromium.googlesource.com/webm/libvpx/+/refs/heads/main/vpx/vp8cx.h#426
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

static XmfVpxEncoderError xmf_vpx_last_create_error = { NO_ERROR };

static void xmf_vpx_set_last_create_error(XmfVpxEncoderErrorCode code, vpx_codec_err_t vpx_error)
{
	xmf_vpx_last_create_error.code = code;
	xmf_vpx_last_create_error.detail.vpx_error.error_code = vpx_error;
}


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

/*
 * Calculate tile_columns_log2 based on thread count and frame width.
 *
 * VP9 tiles enable parallel encode/decode. Each tile is independent,
 * so we want roughly 1 tile per thread for maximum parallelism.
 *
 * tile_columns_log2 = 0 -> 1 tile
 * tile_columns_log2 = 1 -> 2 tiles
 * tile_columns_log2 = 2 -> 4 tiles
 * tile_columns_log2 = 3 -> 8 tiles
 *
 * Doc: tile width is limited to 256..4096 pixels.
 * https://chromium.googlesource.com/webm/libvpx/+/refs/heads/main/vpx/vp8cx.h#310
 */
static int xmf_vpx_calc_tile_columns_log2(int threads, uint32_t width)
{
	int desired_tiles = 1;
	int min_tiles_by_width = 1;
	int max_tiles_by_width = 1;
	int candidates[] = { 1, 2, 4, 8 };
	int candidate_count = (int)(sizeof(candidates) / sizeof(candidates[0]));
	int best_tiles = 1;
	int best_distance = 1000000;
	int i = 0;

	if (threads >= 8)
	{
		desired_tiles = 8;
	}
	else if (threads >= 4)
	{
		desired_tiles = 4;
	}
	else if (threads >= 2)
	{
		desired_tiles = 2;
	}

	max_tiles_by_width = (int)(width / 256);
	if (max_tiles_by_width < 1)
	{
		max_tiles_by_width = 1;
	}

	min_tiles_by_width = (int)((width + 4095) / 4096);
	if (min_tiles_by_width < 1)
	{
		min_tiles_by_width = 1;
	}

	for (i = 0; i < candidate_count; i++)
	{
		int tiles = candidates[i];
		int distance = tiles - desired_tiles;

		if (tiles < min_tiles_by_width || tiles > max_tiles_by_width)
		{
			continue;
		}

		if (distance < 0)
		{
			distance = -distance;
		}

		if (distance < best_distance)
		{
			best_distance = distance;
			best_tiles = tiles;
		}
	}

	if (best_tiles < min_tiles_by_width)
	{
		best_tiles = min_tiles_by_width;
	}
	if (best_tiles > max_tiles_by_width)
	{
		best_tiles = max_tiles_by_width;
	}

	if (best_tiles >= 8)
	{
		return 3;
	}
	if (best_tiles >= 4)
	{
		return 2;
	}
	if (best_tiles >= 2)
	{
		return 1;
	}
	return 0;
}

XmfVpxEncoder *XmfVpxEncoder_Create(XmfVpxEncoderConfig config)
{
	xmf_vpx_set_last_create_error(NO_ERROR, VPX_CODEC_OK);

	XmfVpxEncoder *encoder = (XmfVpxEncoder *)malloc(sizeof(XmfVpxEncoder));
	if (!encoder)
	{
		xmf_vpx_set_last_create_error(MEMORY_ERROR, VPX_CODEC_OK);
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
		xmf_vpx_set_last_create_error(INVALID_PARAM, VPX_CODEC_OK);
		return NULL; // Invalid codec parameter
	}

	vpx_codec_err_t res = vpx_codec_enc_config_default(iface, &encoder->cfg, 0);
	if (res != VPX_CODEC_OK)
	{
		free(encoder);
		xmf_vpx_set_last_create_error(VPX_ERROR, res);
		return NULL; // Failed to get default config
	}

	// Set encoding parameters
	// Doc: vpx_codec_enc_cfg_t::g_w / g_h (frame width/height in pixels)
	// https://chromium.googlesource.com/webm/libvpx/+/refs/heads/main/vpx/vpx_encoder.h#383
	encoder->cfg.g_w = config.width;
	encoder->cfg.g_h = config.height;

	// Doc: vpx_codec_enc_cfg_t::rc_target_bitrate (kilobits per second)
	// https://chromium.googlesource.com/webm/libvpx/+/refs/heads/main/vpx/vpx_encoder.h#451
	encoder->cfg.rc_target_bitrate = config.bitrate;

	// Doc: vpx_codec_enc_cfg_t::g_timebase (timebase for pts)
	// https://chromium.googlesource.com/webm/libvpx/+/refs/heads/main/vpx/vpx_encoder.h#314
	encoder->cfg.g_timebase.num = config.timebase_num;
	encoder->cfg.g_timebase.den = config.timebase_den;

	// Doc: vpx_codec_enc_cfg_t::g_error_resilient + VPX_ERROR_RESILIENT_DEFAULT
	// https://chromium.googlesource.com/webm/libvpx/+/refs/heads/main/vpx/vpx_encoder.h#323
	// https://chromium.googlesource.com/webm/libvpx/+/refs/heads/main/vpx/vpx_encoder.h#103
	encoder->cfg.g_error_resilient = VPX_ERROR_RESILIENT_DEFAULT;

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

	// Doc: vpx_codec_enc_cfg_t::rc_end_usage (rate control mode)
	// https://chromium.googlesource.com/webm/libvpx/+/refs/heads/main/vpx/vpx_encoder.h#399
	encoder->cfg.rc_end_usage = VPX_CBR;

	/*
	 * Threading:
	 * - The caller provides a desired thread count via config.threads.
	 * - We clamp it to avoid invalid/unstable values observed in production (e.g. threads=20 causing init failures).
	 * - IMPORTANT: VP9 row-mt is disabled for threads>1 due to crashes on our libvpx build.
	 */
	threads_value = (int)xmf_u32_clamp(config.threads, 1, 8);
	// Doc: vpx_codec_enc_cfg_t::g_threads (maximum number of threads)
	// https://chromium.googlesource.com/webm/libvpx/+/refs/heads/main/vpx/vpx_encoder.h#267
	encoder->cfg.g_threads = (unsigned int)threads_value;

	/*
	 * Realtime defaults:
	 * - g_lag_in_frames must be 0 to disable lookahead. This is the single most important knob for our workload.
	 * - rc_dropframe_thresh lets the encoder drop frames under pressure rather than accumulating latency.
	 * - enable_auto_alt_ref and ARNR are quality features that add work/latency; we disable them.
	 */
	lag_in_frames_value = 0;
	// Doc: vpx_codec_enc_cfg_t::g_lag_in_frames (lookahead/lag; 0 disables)
	// https://chromium.googlesource.com/webm/libvpx/+/refs/heads/main/vpx/vpx_encoder.h#339
	encoder->cfg.g_lag_in_frames = (unsigned int)lag_in_frames_value;

	// allow keyframes at any time
	// Doc: vpx_codec_enc_cfg_t::kf_mode / kf_min_dist / kf_max_dist
	// https://chromium.googlesource.com/webm/libvpx/+/refs/heads/main/vpx/vpx_encoder.h#540
	encoder->cfg.kf_mode = VPX_KF_AUTO;
	encoder->cfg.kf_min_dist = 0;
	encoder->cfg.kf_max_dist = 9999;

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
		tile_columns_log2_value = encoder->codec_type == VP9
			? xmf_vpx_calc_tile_columns_log2(threads_value, config.width)
			: 0;
		tile_rows_log2_value = 0;
		tune_content_value = encoder->codec_type == VP9 ? VP9E_CONTENT_SCREEN : 0;
		screen_content_mode_value = 1;
		break;
	case XMF_VPX_PRESET_SANE:
		dropframe_thresh_value = 5;
		cpuused_value = encoder->codec_type == VP9 ? 6 : 8;
		tile_columns_log2_value = encoder->codec_type == VP9
			? xmf_vpx_calc_tile_columns_log2(threads_value, config.width)
			: 0;
		tile_rows_log2_value = 0;
		tune_content_value = encoder->codec_type == VP9 ? VP9E_CONTENT_SCREEN : 0;
		screen_content_mode_value = 1;
		break;
	case XMF_VPX_PRESET_BEST_PERFORMANCE:
		dropframe_thresh_value = 10;
		cpuused_value = encoder->codec_type == VP9 ? 9 : 12;
		tile_columns_log2_value = encoder->codec_type == VP9
			? xmf_vpx_calc_tile_columns_log2(threads_value, config.width)
			: 0;
		tile_rows_log2_value = 0;
		tune_content_value = encoder->codec_type == VP9 ? VP9E_CONTENT_SCREEN : 0;
		screen_content_mode_value = 2;
		break;
	}

	// Doc: vpx_codec_enc_cfg_t::rc_dropframe_thresh
	// https://chromium.googlesource.com/webm/libvpx/+/refs/heads/main/vpx/vpx_encoder.h#468
	encoder->cfg.rc_dropframe_thresh = (unsigned int)dropframe_thresh_value;

	// Initialize codec
	res = vpx_codec_enc_init(&encoder->codec, iface, &encoder->cfg, 0);
	if (res != VPX_CODEC_OK)
	{
		free(encoder);
		xmf_vpx_set_last_create_error(VPX_ERROR, res);
		return NULL; // Failed to initialize codec
	}

	encoder->pts = 0;
	encoder->lastError.code = NO_ERROR;

	// Note: VP8E_SET_CPUUSED is used for both VP8 and VP9 in libvpx.
	// Doc: VP8E_SET_CPUUSED valid range (VP8: -16..16, VP9: -9..9)
	// https://chromium.googlesource.com/webm/libvpx/+/refs/heads/main/vpx/vp8cx.h#144
	if (xmf_vpx_apply_control(encoder, VP8E_SET_CPUUSED, cpuused_value) != 0)
	{
		xmf_vpx_set_last_create_error(encoder->lastError.code, encoder->lastError.detail.vpx_error.error_code);
		XmfVpxEncoder_Destroy(encoder);
		return NULL;
	}

	// enable_auto_alt_ref enables alternative reference frames.
	// This can improve quality but adds complexity/latency, which we do not want in realtime streaming.
	// Default is disabled for all presets.
	enable_auto_altref_value = 0;
	// Doc: VP8E_SET_ENABLEAUTOALTREF
	// https://chromium.googlesource.com/webm/libvpx/+/refs/heads/main/vpx/vp8cx.h#157
	if (xmf_vpx_apply_control(encoder, VP8E_SET_ENABLEAUTOALTREF, enable_auto_altref_value) != 0)
	{
		xmf_vpx_set_last_create_error(encoder->lastError.code, encoder->lastError.detail.vpx_error.error_code);
		XmfVpxEncoder_Destroy(encoder);
		return NULL;
	}

	if (encoder->codec_type == VP8)
	{
		// VP8 screen content mode tunes the encoder for UI/desktop content.
		// Doc: VP8E_SET_SCREEN_CONTENT_MODE
		// https://chromium.googlesource.com/webm/libvpx/+/refs/heads/main/vpx/vp8cx.h#289
		if (xmf_vpx_apply_control(encoder, VP8E_SET_SCREEN_CONTENT_MODE, screen_content_mode_value) != 0)
		{
			xmf_vpx_set_last_create_error(encoder->lastError.code, encoder->lastError.detail.vpx_error.error_code);
			XmfVpxEncoder_Destroy(encoder);
			return NULL;
		}

		// ARNR is a VP8-centric temporal filtering knob.
		// Keep it disabled for realtime performance.
		arnr_maxframes_value = 0;
		arnr_strength_value = 0;
		arnr_type_value = 1;

		// Doc: VP8E_SET_ARNR_MAXFRAMES / VP8E_SET_ARNR_STRENGTH / VP8E_SET_ARNR_TYPE
		// https://chromium.googlesource.com/webm/libvpx/+/refs/heads/main/vpx/vp8cx.h#206
		if (xmf_vpx_apply_control(encoder, VP8E_SET_ARNR_MAXFRAMES, arnr_maxframes_value) != 0)
		{
			xmf_vpx_set_last_create_error(encoder->lastError.code, encoder->lastError.detail.vpx_error.error_code);
			XmfVpxEncoder_Destroy(encoder);
			return NULL;
		}
		if (xmf_vpx_apply_control(encoder, VP8E_SET_ARNR_STRENGTH, arnr_strength_value) != 0)
		{
			xmf_vpx_set_last_create_error(encoder->lastError.code, encoder->lastError.detail.vpx_error.error_code);
			XmfVpxEncoder_Destroy(encoder);
			return NULL;
		}
		if (xmf_vpx_apply_control(encoder, VP8E_SET_ARNR_TYPE, arnr_type_value) != 0)
		{
			xmf_vpx_set_last_create_error(encoder->lastError.code, encoder->lastError.detail.vpx_error.error_code);
			XmfVpxEncoder_Destroy(encoder);
			return NULL;
		}
	}

	if (encoder->codec_type == VP9)
	{
		// VP9 row-multithreading (row-mt) is forced OFF for stability (see note above).
		row_mt_value = 0;
		// Doc: VP9E_SET_ROW_MT
		// https://chromium.googlesource.com/webm/libvpx/+/refs/heads/main/vpx/vp8cx.h#512
		if (xmf_vpx_apply_control(encoder, VP9E_SET_ROW_MT, row_mt_value) != 0)
		{
			xmf_vpx_set_last_create_error(encoder->lastError.code, encoder->lastError.detail.vpx_error.error_code);
			XmfVpxEncoder_Destroy(encoder);
			return NULL;
		}

		// Doc: VP9E_SET_TILE_COLUMNS (log2 of tile columns; tile width 256..4096)
		// https://chromium.googlesource.com/webm/libvpx/+/refs/heads/main/vpx/vp8cx.h#310
		if (xmf_vpx_apply_control(encoder, VP9E_SET_TILE_COLUMNS, tile_columns_log2_value) != 0)
		{
			xmf_vpx_set_last_create_error(encoder->lastError.code, encoder->lastError.detail.vpx_error.error_code);
			XmfVpxEncoder_Destroy(encoder);
			return NULL;
		}

		// Doc: VP9E_SET_TILE_ROWS (log2 of tile rows)
		// https://chromium.googlesource.com/webm/libvpx/+/refs/heads/main/vpx/vp8cx.h#333
		if (xmf_vpx_apply_control(encoder, VP9E_SET_TILE_ROWS, tile_rows_log2_value) != 0)
		{
			xmf_vpx_set_last_create_error(encoder->lastError.code, encoder->lastError.detail.vpx_error.error_code);
			XmfVpxEncoder_Destroy(encoder);
			return NULL;
		}

		// Doc: VP9E_SET_TUNE_CONTENT (content type: DEFAULT/SCREEN/FILM)
		// https://chromium.googlesource.com/webm/libvpx/+/refs/heads/main/vpx/vp8cx.h#426
		if (xmf_vpx_apply_control(encoder, VP9E_SET_TUNE_CONTENT, tune_content_value) != 0)
		{
			xmf_vpx_set_last_create_error(encoder->lastError.code, encoder->lastError.detail.vpx_error.error_code);
			XmfVpxEncoder_Destroy(encoder);
			return NULL;
		}

#ifdef VP9E_SET_FRAME_PARALLEL_DECODING
		// Keep frame-parallel decoding enabled for better decoder parallelism.
		frame_parallel_decoding_value = 1;
		// Doc: VP9E_SET_FRAME_PARALLEL_DECODING
		// https://chromium.googlesource.com/webm/libvpx/+/refs/heads/main/vpx/vp8cx.h#352
		if (xmf_vpx_apply_control(encoder, VP9E_SET_FRAME_PARALLEL_DECODING, frame_parallel_decoding_value) != 0)
		{
			xmf_vpx_set_last_create_error(encoder->lastError.code, encoder->lastError.detail.vpx_error.error_code);
			XmfVpxEncoder_Destroy(encoder);
			return NULL;
		}
#endif
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

XmfVpxEncoderError XmfVpxEncoder_GetLastCreateError(void)
{
	return xmf_vpx_last_create_error;
}



