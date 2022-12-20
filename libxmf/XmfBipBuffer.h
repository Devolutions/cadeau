#ifndef XMF_BIP_BUFFER_H
#define XMF_BIP_BUFFER_H

#include <xmf/xmf.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct xmf_bip_block XmfBipBlock;
typedef struct xmf_bip_buffer XmfBipBuffer;

XMF_EXPORT bool XmfBipBuffer_Grow(XmfBipBuffer* ctx, size_t size);
XMF_EXPORT void XmfBipBuffer_Clear(XmfBipBuffer* ctx);

XMF_EXPORT size_t XmfBipBuffer_UsedSize(XmfBipBuffer* ctx);
XMF_EXPORT size_t XmfBipBuffer_BufferSize(XmfBipBuffer* ctx);

XMF_EXPORT uint8_t* XmfBipBuffer_WriteReserve(XmfBipBuffer* ctx, size_t size);
XMF_EXPORT uint8_t* XmfBipBuffer_WriteTryReserve(XmfBipBuffer* ctx, size_t size, size_t* reserved);
XMF_EXPORT void XmfBipBuffer_WriteCommit(XmfBipBuffer* ctx, size_t size);

XMF_EXPORT uint8_t* XmfBipBuffer_ReadReserve(XmfBipBuffer* ctx, size_t size);
XMF_EXPORT uint8_t* XmfBipBuffer_ReadTryReserve(XmfBipBuffer* ctx, size_t size, size_t* reserved);
XMF_EXPORT void XmfBipBuffer_ReadCommit(XmfBipBuffer* ctx, size_t size);

XMF_EXPORT int XmfBipBuffer_Read(XmfBipBuffer* ctx, uint8_t* data, size_t size);
XMF_EXPORT int XmfBipBuffer_Write(XmfBipBuffer* ctx, const uint8_t* data, size_t size);

XMF_EXPORT void XmfBipBuffer_SetSignaledState(XmfBipBuffer* ctx, bool signaled);
XMF_EXPORT bool XmfBipBuffer_GetSignaledState(XmfBipBuffer* ctx);

XMF_EXPORT XmfBipBuffer* XmfBipBuffer_New(size_t size);
XMF_EXPORT void XmfBipBuffer_Free(XmfBipBuffer* ctx);

#ifdef __cplusplus
}
#endif

#endif /* XMF_BIP_BUFFER_H */