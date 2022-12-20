
#include "XmfMkvWriter.h"

struct _XmfMkvWriter
{
   IXmfMkvWriterVtbl* vtbl;

   FILE* fp;
   bool owner;
   XmfBipBuffer* bb;
   int64_t position;
};

int32_t XmfMkvWriter_Write(XmfMkvWriter* This, const void* buffer, uint32_t length)
{
    if (length == 0)
        return 0;

    if (buffer == NULL)
        return -1;

    if (This->fp) {
        size_t status = fwrite(buffer, 1, length, This->fp);
        return (status == length) ? 0 : -1;
    } else if (This->bb) {
        size_t safeSize = XmfBipBuffer_UsedSize(This->bb) + (size_t) length;
        if ((safeSize % 4096) != 0) {
            safeSize += 4096 - (safeSize % 4096);
        }
        XmfBipBuffer_Grow(This->bb, safeSize);
        int status = XmfBipBuffer_Write(This->bb, buffer, (size_t) length);
        if (status == length) {
            This->position += length;
            return 0;
        }
        return -1;
    }

    return -1;
}

int64_t XmfMkvWriter_GetPosition(XmfMkvWriter* This)
{
    if (This->fp) {
        return (int64_t) XmfFile_Tell(This->fp);
    } else if (This->bb) {
        return This->position;
    }

    return -1;
}

int32_t XmfMkvWriter_SetPosition(XmfMkvWriter* This, int64_t position)
{
    if (This->fp) {
        return (int32_t) XmfFile_Seek(This->fp, (uint64_t) position, SEEK_SET);
    }

    return -1;
}

bool XmfMkvWriter_Seekable(XmfMkvWriter* This)
{
    return false;

    if (This->fp) {
        return true;
    }

    return false;
}

void XmfMkvWriter_ElementStartNotify(XmfMkvWriter* This, uint64_t element_id, int64_t position)
{
    
}

void* XmfMkvWriter_Dispose(XmfMkvWriter* This, bool freeMem)
{
    XmfMkvWriter_Close(This);

    if (freeMem) {
        free(This);
    }

    return This;
}

// extended functions

bool XmfMkvWriter_Open(XmfMkvWriter* This, const char* filename)
{
    This->fp = XmfFile_Open(filename, "wb");
    This->owner = true;
    return This->fp ? true : false;
}

void XmfMkvWriter_Close(XmfMkvWriter* This)
{
    if (This->fp) {
        if (This->owner) {
            XmfFile_Close(This->fp);
            This->fp = NULL;
        }
    }
}

void XmfMkvWriter_SetFilePointer(XmfMkvWriter* This, FILE* fp)
{
    This->fp = fp;
}

void XmfMkvWriter_SetBipBuffer(XmfMkvWriter* This, XmfBipBuffer* bb)
{
    This->bb = bb;
}

static IXmfMkvWriterVtbl g_IXmfMkvWriterVtbl = {
    (void*) XmfMkvWriter_Write,
    (void*) XmfMkvWriter_GetPosition,
    (void*) XmfMkvWriter_SetPosition,
    (void*) XmfMkvWriter_Seekable,
    (void*) XmfMkvWriter_ElementStartNotify,
    (void*) XmfMkvWriter_Dispose
};

XmfMkvWriter* XmfMkvWriter_New()
{
    XmfMkvWriter* This = (XmfMkvWriter*) calloc(1, sizeof(XmfMkvWriter));

    if (!This)
        return NULL;

    This->vtbl = &g_IXmfMkvWriterVtbl;

    return This;
}
