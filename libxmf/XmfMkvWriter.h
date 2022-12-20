#ifndef XMF_MKV_WRITER_H
#define XMF_MKV_WRITER_H

#include <xmf/xmf.h>

#include "XmfFile.h"
#include "XmfBipBuffer.h"

#ifdef __cplusplus
extern "C" {
#endif

// interface definition

typedef struct _IXmfMkvWriter IXmfMkvWriter;

typedef struct
{
    // Writes out |len| bytes of |buf|. Returns 0 on success.
    int32_t (*Write)(IXmfMkvWriter* This, const void* buf, uint32_t len);

    // Returns the offset of the output position from the beginning of the output.
    int64_t (*GetPosition)(IXmfMkvWriter* This);

    // Set the current File position. Returns 0 on success.
    int32_t (*SetPosition)(IXmfMkvWriter* This, int64_t position);

    // Returns true if the writer is seekable.
    bool (*Seekable)(IXmfMkvWriter* This);

    // Element start notification. Called whenever an element identifier is about
    // to be written to the stream. |element_id| is the element identifier, and
    // |position| is the location in the WebM stream where the first octet of the
    // element identifier will be written.
    // Note: the |MkvId| enumeration in webmids.hpp defines element values.
    void (*ElementStartNotify)(IXmfMkvWriter* This, uint64_t element_id, int64_t position);

    // virtual destructor
    void* (*Dispose)(IXmfMkvWriter* This, bool free);
} IXmfMkvWriterVtbl;

struct _IXmfMkvWriter
{
   IXmfMkvWriterVtbl* vtbl;
};

// class definition

typedef struct _XmfMkvWriter XmfMkvWriter;

bool XmfMkvWriter_Open(XmfMkvWriter* This, const char* filename);
void XmfMkvWriter_Close(XmfMkvWriter* This);

void XmfMkvWriter_SetFilePointer(XmfMkvWriter* This, FILE* fp);
void XmfMkvWriter_SetBipBuffer(XmfMkvWriter* This, XmfBipBuffer* bb);

XmfMkvWriter* XmfMkvWriter_New();

#ifdef __cplusplus
}
#endif

#endif /* XMF_MKV_WRITER_H */