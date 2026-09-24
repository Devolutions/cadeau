#ifndef XMF_VPX_IMAGE_H
#define XMF_VPX_IMAGE_H

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
     * Returns the libvpx vpx_img_fmt_t of the decoded image, or VPX_IMG_FMT_NONE
     * when the image is unavailable. 8-bit I420 is VPX_IMG_FMT_I420; high bit depth
     * images carry the VPX_IMG_FMT_HIGHBITDEPTH flag.
     */
    XMF_EXPORT int XmfVpxImage_GetFormat(const XmfVpxImage *image);

    /**
     * Returns the start of a plane (VPX_PLANE_Y, VPX_PLANE_U, VPX_PLANE_V or
     * VPX_PLANE_ALPHA), or NULL when the image or plane index is unavailable.
     *
     * The memory belongs to the decoder and stays valid only until its next call.
     * For I420, the chroma planes have (width + 1) / 2 columns and (height + 1) / 2 rows.
     */
    XMF_EXPORT const uint8_t *XmfVpxImage_GetPlane(const XmfVpxImage *image, int plane);

    /**
     * Returns the distance in bytes between rows of a plane, or 0 when the image or
     * plane index is unavailable.
     */
    XMF_EXPORT int XmfVpxImage_GetStride(const XmfVpxImage *image, int plane);

    /**
     * Returns the libvpx vpx_color_space_t reported by the decoder. VP8 carries no
     * color metadata, so VP8 images report VPX_CS_UNKNOWN.
     */
    XMF_EXPORT int XmfVpxImage_GetColorSpace(const XmfVpxImage *image);

    /**
     * Returns the libvpx vpx_color_range_t reported by the decoder. VP8 images
     * report VPX_CR_STUDIO_RANGE.
     */
    XMF_EXPORT int XmfVpxImage_GetColorRange(const XmfVpxImage *image);

#ifdef __cplusplus
}
#endif
#endif // XMF_VPX_IMAGE_H
