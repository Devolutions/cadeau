#ifndef XMF_VPXDECODER_H
#define XMF_VPXDECODER_H

#include <stdint.h>
#include "xmf/xmf.h"
#include "XmfVpxImage.h"

typedef struct xmf_vpx_decoder XmfVpXDecoder;

typedef enum
{
    VP8,
    VP9,
} XmfVpxCodecType;

typedef struct xmf_vpx_decoder_config
{
    unsigned int threads; /**< Maximum number of threads to use, default 1 */
    unsigned int w;       /**< Width (set to 0 if unknown) */
    unsigned int h;       /**< Height (set to 0 if unknown) */
    XmfVpxCodecType codec;
} XmfVpxDecoderConfig;

typedef enum xmf_vpx_decoder_error_code
{
    NO_ERROR = 0,
    MEMORY_ERROR = 1,
    INIT_ERROR = 2,
    DECODE_ERROR = 3,
    NO_FRAME_AVAILABLE = 4,
    VPX_ERROR = 5,
} XmfVpxDecoderErrorCode;

typedef struct xmf_vpx_decoder_error
{
    XmfVpxDecoderErrorCode code;
    union
    {
        struct
        {
            int error_code;
        } vpx_error;
    } detail;
} XmfVpxDecoderError;

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Creates and initializes a VPX decoder.
     *
     * @param cfg Pointer to the decoder configuration.
     * @return Pointer to the decoder instance, or NULL on failure.
     */
    XMF_EXPORT XmfVpXDecoder *XmfVpxDecoder_Create(XmfVpxDecoderConfig cfg);

    /**
     * @brief Decodes compressed data.
     *
     * @param ctx Pointer to the decoder instance.
     * @param data Compressed data buffer.
     * @param size Size of the data buffer.
     * @return 0 on success, -1 on failure.
     */
    XMF_EXPORT int XmfVpxDecoder_Decode(XmfVpXDecoder *ctx, const uint8_t *data, unsigned int size);

    /**
     * @brief Retrieves the next decoded frame.
     *
     * @param ctx Pointer to the decoder instance.
     * @return Pointer to the decoded image, or NULL if no frame is available.
     */
    XMF_EXPORT XmfVpxImage *XmfVpxDecoder_GetNextFrame(XmfVpXDecoder *ctx);

    /**
     * @brief Retrieves the last error that occurred in the decoder.
     *
     * @param ctx Pointer to the decoder instance.
     * @return The last error information.
     */
    XMF_EXPORT XmfVpxDecoderError XmfVpxDecoder_GetLastError(XmfVpXDecoder *ctx);

    /**
     * @brief Destroys the decoder and frees associated resources.
     *
     * @param ctx Pointer to the decoder instance.
     */
    XMF_EXPORT void XmfVpxDecoder_Destroy(XmfVpXDecoder *ctx);

#ifdef __cplusplus
}
#endif

#endif /* XMF_VPXDECODER_H */
