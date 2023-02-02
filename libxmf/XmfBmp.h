#ifndef XMF_BMP_H
#define XMF_BMP_H

#include <xmf/xmf.h>

#ifdef __cplusplus
extern "C" {
#endif

XMF_EXPORT bool XmfBmp_SaveFile(const char* filename, const uint8_t* data, uint32_t width, uint32_t height, uint32_t step);

XMF_EXPORT bool XmfBmp_LoadFile(const char* filename, uint8_t** data, uint32_t* width, uint32_t* height, uint32_t* step);

#ifdef __cplusplus
}
#endif

#endif /* XMF_BMP_H */
