#ifndef XMF_IMAGE_H
#define XMF_IMAGE_H

#include <xmf/xmf.h>

#ifdef __cplusplus
extern "C" {
#endif

XMF_EXPORT bool XmfImage_LoadFile(const char* filename, uint8_t** data, uint32_t* width, uint32_t* height, uint32_t* step);

XMF_EXPORT bool XmfImage_SaveFile(const char* filename, const uint8_t* data, uint32_t width, uint32_t height, uint32_t step);

XMF_EXPORT void XmfImage_FreeData(uint8_t* data);

#ifdef __cplusplus
}
#endif

#endif /* XMF_IMAGE_H */
