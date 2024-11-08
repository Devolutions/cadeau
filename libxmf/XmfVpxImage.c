#include "XmfVpxImage.h"
#include <stdlib.h>
#include <vpx/vpx_image.h>

struct xmf_vpx_image
{
    vpx_image_t *img;
};

XmfVpxImage *XmfVpxImage_Create()
{
    XmfVpxImage *image = (XmfVpxImage *)malloc(sizeof(XmfVpxImage));
    if (image)
    {
        image->img = NULL;
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
