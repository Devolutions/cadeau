#ifndef XMF_VPX_IMAGE_H
#define XMF_VPX_IMAGE_H

#include <stdint.h>
#include <vpx/vpx_image.h>
#include <xmf/xmf.h>

/// <div rustbindgen opaque></div>
typedef struct xmf_vpx_image XmfVpxImage;

#ifdef __cplusplus
extern "C"
{
#endif

    XmfVpxImage *XmfVpxImage_Create(vpx_image_t *data);
    void XmfVpxImage_SetData(XmfVpxImage *image, vpx_image_t *data);
    vpx_image_t *XmfVpxImage_GetData(const XmfVpxImage *image);

    XMF_EXPORT void XmfVpxImage_Destroy(XmfVpxImage *image);
    XMF_EXPORT unsigned int XmfVpxImage_GetWidth(const XmfVpxImage *image);
    XMF_EXPORT unsigned int XmfVpxImage_GetHeight(const XmfVpxImage *image);

    /**
     * @brief Retrieves the pixel format of a decoded image.
     *
     * 8-bit I420 is VPX_IMG_FMT_I420. High bit depth formats carry the VPX_IMG_FMT_HIGHBITDEPTH flag.
     *
     * @param image Pointer to the decoded image.
     * @return The libvpx vpx_img_fmt_t value, or VPX_IMG_FMT_NONE if the image is unavailable.
     */
    XMF_EXPORT int XmfVpxImage_GetFormat(const XmfVpxImage *image);

    /**
     * @brief Retrieves the start of a plane of a decoded image.
     *
     * The memory belongs to the decoder and stays valid only until the next XmfVpxDecoder_Decode call
     * or until the decoder is destroyed.
     * For I420, the chroma planes have (width + 1) / 2 columns and (height + 1) / 2 rows.
     *
     * @param image Pointer to the decoded image.
     * @param plane Plane index: VPX_PLANE_Y, VPX_PLANE_U, VPX_PLANE_V or VPX_PLANE_ALPHA.
     * @return Pointer to the first byte of the plane, or NULL if the image or plane is unavailable.
     */
    XMF_EXPORT const uint8_t *XmfVpxImage_GetPlane(const XmfVpxImage *image, int plane);

    /**
     * @brief Retrieves the row stride of a plane of a decoded image.
     *
     * @param image Pointer to the decoded image.
     * @param plane Plane index: VPX_PLANE_Y, VPX_PLANE_U, VPX_PLANE_V or VPX_PLANE_ALPHA.
     * @return Distance in bytes between the starts of two rows, or 0 if the image or plane is unavailable.
     */
    XMF_EXPORT int XmfVpxImage_GetStride(const XmfVpxImage *image, int plane);

    /**
     * @brief Retrieves the color space reported by the decoder.
     *
     * VP8 carries no color metadata, so VP8 images report VPX_CS_UNKNOWN.
     *
     * @param image Pointer to the decoded image.
     * @return The libvpx vpx_color_space_t value, or VPX_CS_UNKNOWN if the image is unavailable.
     */
    XMF_EXPORT int XmfVpxImage_GetColorSpace(const XmfVpxImage *image);

    /**
     * @brief Retrieves the color range reported by the decoder.
     *
     * VP8 images report VPX_CR_STUDIO_RANGE.
     *
     * @param image Pointer to the decoded image.
     * @return The libvpx vpx_color_range_t value, or -1 if the image is unavailable.
     */
    XMF_EXPORT int XmfVpxImage_GetColorRange(const XmfVpxImage *image);

#ifdef __cplusplus
}
#endif
#endif // XMF_VPX_IMAGE_H
