#ifndef XMF_VPXENCODER_H
#define XMF_VPXENCODER_H

#include <stdint.h>
#include "vpx/vpx_codec.h"
#include "XmfVpxImage.h"
#include "xmf/xmf.h"
#include "XmfVpxPacket.h"

typedef struct xmf_vpx_encoder XmfVpxEncoder;

typedef enum
{
    VP8,
    VP9
} XmfVpxCodecType;

typedef enum
{
    /**
     * Default profile: realtime-friendly but conservative.
     *
     * Intended for general-purpose use where stability is the priority.
     */
    XMF_VPX_PRESET_DEFAULT = 0,

    /**
     * Sane profile: realtime oriented.
     *
     * Trades quality for speed, but avoids the most aggressive settings.
     */
    XMF_VPX_PRESET_SANE = 1,

    /**
     * Best performance profile: maximum realtime throughput.
     *
     * Strongly prioritizes speed and low latency over quality/bitrate efficiency.
     */
    XMF_VPX_PRESET_BEST_PERFORMANCE = 2,
} XmfVpxEncoderPreset;

typedef struct
{
    XmfVpxCodecType codec;
    uint32_t width;
    uint32_t height;
    uint32_t bitrate;
    int32_t timebase_num;
    int32_t timebase_den;
    uint32_t threads;
    /**
     * Realtime preset selection.
     *
     * This is the supported way to tune encoder performance.
     * The library does not rely on environment variables for behavior changes.
     */
    XmfVpxEncoderPreset preset;
} XmfVpxEncoderConfig;

/** Marks an XmfVpxEncoderQuantizerRange field as "keep the libvpx default". */
#define XMF_VPX_UNSET ((int32_t)0x80000000)

/**
 * Optional quantizer range for XmfVpxEncoder_CreateEx.
 *
 * Set struct_size to sizeof(XmfVpxEncoderQuantizerRange) and every field you do not override to XMF_VPX_UNSET.
 * 0 is a valid quantizer, so a zero-initialized struct is not the same as XmfVpxEncoder_Create.
 * With every field unset, the encoder is configured exactly like XmfVpxEncoder_Create.
 */
typedef struct
{
    /** sizeof(XmfVpxEncoderQuantizerRange), so fields can be appended later. */
    uint32_t struct_size;
    /** vpx_codec_enc_cfg_t::rc_min_quantizer (0..63). Must not exceed the effective max quantizer. */
    int32_t min_quantizer;
    /**
     * vpx_codec_enc_cfg_t::rc_max_quantizer (0..63, libvpx default 63).
     *
     * Lower values keep frames sharp when the bitrate budget is tight (e.g. sparse frames), at the cost of size.
     */
    int32_t max_quantizer;
} XmfVpxEncoderQuantizerRange;

typedef enum
{
    NO_ERROR,
    MEMORY_ERROR,
    VPX_ERROR,
    INVALID_PARAM,
} XmfVpxEncoderErrorCode;

typedef struct
{
    XmfVpxEncoderErrorCode code;
    union
    {
        struct
        {
            vpx_codec_err_t error_code;
        } vpx_error;
    } detail;
} XmfVpxEncoderError;

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Initializes the VPX encoder with the specified configuration.
     *
     * @param config  Pointer to the encoder configuration.
     * @return Pointer to the encoder instance, or NULL on failure.
     */
    XMF_EXPORT XmfVpxEncoder *XmfVpxEncoder_Create(XmfVpxEncoderConfig config);

    /**
     * Same as XmfVpxEncoder_Create, with an optional quantizer range.
     *
     * @param config          Encoder configuration.
     * @param quantizer_range Optional quantizer range, or NULL for XmfVpxEncoder_Create's behavior.
     * @return Pointer to the encoder instance, or NULL on failure.
     *         An invalid quantizer range fails with INVALID_PARAM (see XmfVpxEncoder_GetLastCreateError).
     */
    XMF_EXPORT XmfVpxEncoder *XmfVpxEncoder_CreateEx(XmfVpxEncoderConfig config, const XmfVpxEncoderQuantizerRange *quantizer_range);

    /**
     * Encodes a frame and returns the compressed data.
     *
     * @param encoder  Pointer to the encoder instance.
     * @param image    Pointer to the image to encode.
     * @param pts      Presentation timestamp of the frame.
     * @param duration Duration to show the frame.
     * @param flags    Flags for encoding (e.g., keyframe).
     * @return 0 on success, non-zero on failure.
     */
    XMF_EXPORT int XmfVpxEncoder_EncodeFrame(XmfVpxEncoder *encoder, const XmfVpxImage *image, vpx_codec_pts_t pts, unsigned long duration, unsigned int flags);

    /**
     * Retrieves the compressed frame data.
     *
     * @param encoder      Pointer to the encoder instance.
     * @param output       Pointer to the output buffer, Must be freed by the caller.
     * @param output_size  Pointer to the output buffer size, in bytes.
     * @return 0 on success, non-zero on failure.
     */
    XMF_EXPORT int XmfVpxEncoder_GetEncodedFrame(XmfVpxEncoder *encoder, uint8_t **output, size_t *output_size);

    /**
     * Frees the encoded frame data.
     *
     * @param output Pointer to the output buffer.
     * @return 0 on success, non-zero on failure.
     */
    XMF_EXPORT void XmfVpxEncoder_FreeEncodedFrame(uint8_t *output);
    /**
     * Flushes the encoder, ensuring all frames are encoded.
     *
     * @param encoder Pointer to the encoder instance.
     * @return 0 on success, non-zero on failure.
     */
    XMF_EXPORT int XmfVpxEncoder_Flush(XmfVpxEncoder *encoder);

    /**
     * Releases resources associated with the encoder.
     *
     * @param encoder Pointer to the encoder instance.
     * @return 0 on success, non-zero on failure.
     */
    XMF_EXPORT int XmfVpxEncoder_Destroy(XmfVpxEncoder *encoder);

    /**
     * Retrieves the last error that occurred in the encoder.
     *
     * @param encoder Pointer to the encoder instance.
     * @return The last error code and details.
     */
    XMF_EXPORT XmfVpxEncoderError XmfVpxEncoder_GetLastError(const XmfVpxEncoder *encoder);

    /**
     * Retrieves the last error that occurred during encoder creation.
     *
     * @return The last create error code and details.
     */
    XMF_EXPORT XmfVpxEncoderError XmfVpxEncoder_GetLastCreateError(void);

    /**
     * Retrieves the next packet from the encoder.
     * @param encoder Pointer to the encoder instance.
     *
     * @return 0 on success, non-zero on failure.
     */
    XMF_EXPORT XmfVpxPacket *XmfVpxEncoder_GetPacket(XmfVpxEncoder *encoder, vpx_codec_iter_t *iter);

#ifdef __cplusplus
}
#endif

#endif /* XMF_VPXENCODER_H */
