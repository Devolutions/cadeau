#ifndef XMF_PNG_H
#define XMF_PNG_H

#include <xmf/xmf.h>

#ifdef __cplusplus
extern "C" {
#endif

XMF_EXPORT bool XmfPng_SaveFile(const char* filename, const uint8_t* data, uint32_t width, uint32_t height, uint32_t step);

XMF_EXPORT bool XmfPng_LoadFile(const char* filename, uint8_t** data, uint32_t* width, uint32_t* height, uint32_t* step);

#ifdef __cplusplus
}
#endif

#endif /* XMF_PNG_H */
