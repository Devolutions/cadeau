#ifndef XMF_NAMED_PIPE_H
#define XMF_NAMED_PIPE_H

#include <xmf/xmf.h>

typedef void XmfNamedPipe;

#ifdef __cplusplus
extern "C" {
#endif

int XmfNamedPipe_Read(XmfNamedPipe* np_handle, uint8_t* data, size_t size);
int XmfNamedPipe_Write(XmfNamedPipe* np_handle, const uint8_t* data, size_t size);

XmfNamedPipe* XmfNamedPipe_Open(const char* np_name);
XmfNamedPipe* XmfNamedPipe_Create(const char* np_name);

void XmfNamedPipe_Close(XmfNamedPipe* np_handle);

#ifdef __cplusplus
}
#endif

#endif /* XMF_NAMED_PIPE_H */
