#include "XmfVpxImage.h"
#include <stdlib.h>
#include <vpx/vpx_image.h>

struct xmf_vpx_image
{
    vpx_image_t *img;
};

XmfVpxImage *XmfVpxImage_Create(vpx_image_t *data)
{
    XmfVpxImage *image = (XmfVpxImage *)malloc(sizeof(XmfVpxImage));
    if (image)
    {
        image->img = data;
    }
    return image;
}

void XmfVpxImage_Destroy(XmfVpxImage *image)
{
    if (image)
    {
        free(image);
    }
}

void XmfVpxImage_SetData(XmfVpxImage *image, vpx_image_t *data)
{
    if (image)
    {
        image->img = data;
    }
}

vpx_image_t *XmfVpxImage_GetData(const XmfVpxImage *image)
{
    return image ? image->img : NULL;
}

unsigned int XmfVpxImage_GetWidth(const XmfVpxImage *image)
{
    return image && image->img ? image->img->d_w : 0;
}

unsigned int XmfVpxImage_GetHeight(const XmfVpxImage *image)
{
    return image && image->img ? image->img->d_h : 0;
}

int XmfVpxImage_GetFormat(const XmfVpxImage *image)
{
    return image && image->img ? (int)image->img->fmt : VPX_IMG_FMT_NONE;
}

const uint8_t *XmfVpxImage_GetPlane(const XmfVpxImage *image, int plane)
{
    return image && image->img && plane >= VPX_PLANE_Y && plane <= VPX_PLANE_ALPHA ? image->img->planes[plane] : NULL;
}

int XmfVpxImage_GetStride(const XmfVpxImage *image, int plane)
{
    return image && image->img && plane >= VPX_PLANE_Y && plane <= VPX_PLANE_ALPHA ? image->img->stride[plane] : 0;
}

int XmfVpxImage_GetColorSpace(const XmfVpxImage *image)
{
    return image && image->img ? (int)image->img->cs : VPX_CS_UNKNOWN;
}

int XmfVpxImage_GetColorRange(const XmfVpxImage *image)
{
    return image && image->img ? (int)image->img->range : VPX_CR_STUDIO_RANGE;
}
