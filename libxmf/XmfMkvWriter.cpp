
#include "XmfMkvWriter.h"

struct _XmfMkvWriter
{
   IXmfMkvWriterVtbl* vtbl;

   FILE* fp;
   bool owner;
   XmfBipBuffer* bb;
   XmfNamedPipe* np;
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
        int status = XmfBipBuffer_Write(This->bb, (const uint8_t*) buffer, (size_t) length);
        if (status == length) {
            This->position += length;
            return 0;
        }
        return -1;
    } else if (This->np) {
        int status = XmfNamedPipe_Write(This->np, (const uint8_t*) buffer, length);
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
        This->position = (int64_t) XmfFile_Tell(This->fp);
    } else if (This->bb) {

    } else if (This->np) {

    }

    return This->position;
}

int32_t XmfMkvWriter_SetPosition(XmfMkvWriter* This, int64_t position)
{
    if (This->fp) {
        XmfFile_Seek(This->fp, (uint64_t) position, SEEK_SET);
        This->position = position;
    } else if (This->bb) {
        This->position = position;
    } else if (This->np) {
        This->position = position;
    }

    return 0;
}

bool XmfMkvWriter_Seekable(XmfMkvWriter* This)
{
    if (This->fp) {
        return true;
    } else if (This->bb) {
        return false;
    } else if (This->np) {
        return false;
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
    } else if (This->bb) {

    } else if (This->np) {
        XmfNamedPipe_Close(This->np);
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

void XmfMkvWriter_SetNamedPipe(XmfMkvWriter* This, XmfNamedPipe* np)
{
    This->np = np;
}

static IXmfMkvWriterVtbl g_IXmfMkvWriterVtbl = {
    (int32_t(*)(IXmfMkvWriter*, const void*, uint32_t)) XmfMkvWriter_Write,
#ifdef _WIN32
    (int64_t(*)(IXmfMkvWriter*)) XmfMkvWriter_SetPosition,
    (int32_t(*)(IXmfMkvWriter*, int64_t)) XmfMkvWriter_GetPosition,
#else
    (int64_t(*)(IXmfMkvWriter*)) XmfMkvWriter_GetPosition,
    (int32_t(*)(IXmfMkvWriter*, int64_t)) XmfMkvWriter_SetPosition,
#endif
    (bool(*)(IXmfMkvWriter*)) XmfMkvWriter_Seekable,
    (void(*)(IXmfMkvWriter*, uint64_t, int64_t)) XmfMkvWriter_ElementStartNotify,
    (void*(*)(IXmfMkvWriter*, bool)) XmfMkvWriter_Dispose
};

XmfMkvWriter* XmfMkvWriter_New()
{
    XmfMkvWriter* This = (XmfMkvWriter*) calloc(1, sizeof(XmfMkvWriter));

    if (!This)
        return NULL;

    This->vtbl = &g_IXmfMkvWriterVtbl;

    return This;
}
