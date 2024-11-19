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

#ifdef __cplusplus
}
#endif
#endif // XMF_VPX_IMAGE_H