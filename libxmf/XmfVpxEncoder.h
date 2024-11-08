#ifndef XMF_VPXENCODER_H
#define XMF_VPXENCODER_H

#include <stdint.h>
#include "XmfVpxImage.h"
#include "xmf/xmf.h"

typedef struct xmf_vpx_encoder XmfVpxEncoder;

typedef enum
{
    VP8,
    VP9
} XmfVpxCodecType;

typedef struct
{
    XmfVpxCodecType codec;
    unsigned int width;
    unsigned int height;
    unsigned int bitrate;
    unsigned int timebase_num;
    unsigned int timebase_den;
    unsigned int threads;
} XmfVpxEncoderConfig;

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
            int error_code;
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
     * Encodes a frame and returns the compressed data.
     *
     * @param encoder  Pointer to the encoder instance.
     * @param image    Pointer to the image to encode.
     * @param pts      Presentation timestamp of the frame.
     * @param duration Duration to show the frame.
     * @param flags    Flags for encoding (e.g., keyframe).
     * @return 0 on success, non-zero on failure.
     */
    XMF_EXPORT int XmfVpxEncoder_EncodeFrame(XmfVpxEncoder *encoder, const XmfVpxImage *image, int64_t pts, int64_t duration, unsigned int flags);

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

#ifdef __cplusplus
}
#endif

#endif /* XMF_VPXENCODER_H */
