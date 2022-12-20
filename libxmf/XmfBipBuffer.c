
#include "XmfBipBuffer.h"

/**
 * The Bip Buffer - The Circular Buffer with a Twist:
 * http://www.codeproject.com/Articles/3479/The-Bip-Buffer-The-Circular-Buffer-with-a-Twist
 */

struct xmf_bip_block
{
    size_t index;
    size_t size;
};

struct xmf_bip_buffer
{
    size_t size;
    uint8_t* buffer;
    size_t pageSize;
    bool signaled;
    XmfBipBlock blockA;
    XmfBipBlock blockB;
    XmfBipBlock readR;
    XmfBipBlock writeR;
};

#define XmfBipBlock_Clear(_bbl) \
    _bbl.index = _bbl.size = 0

#define XmfBipBlock_Copy(_dst, _src) \
    _dst.index = _src.index; \
    _dst.size = _src.size

void XmfBipBuffer_Clear(XmfBipBuffer* ctx)
{
    XmfBipBlock_Clear(ctx->blockA);
    XmfBipBlock_Clear(ctx->blockB);
    XmfBipBlock_Clear(ctx->readR);
    XmfBipBlock_Clear(ctx->writeR);
}

bool XmfBipBuffer_AllocBuffer(XmfBipBuffer* ctx, size_t size)
{
    if (size < 1)
        return false;

    size += size % ctx->pageSize;

    ctx->buffer = (uint8_t*) malloc(size);

    if (!ctx->buffer)
        return false;

    ctx->size = size;

    return true;
}

bool XmfBipBuffer_Grow(XmfBipBuffer* ctx, size_t size)
{
    uint8_t* block;
    uint8_t* buffer;
    size_t blockSize = 0;
    size_t commitSize = 0;

    size += size % ctx->pageSize;

    if (size <= ctx->size)
        return true;

    buffer = (uint8_t*) malloc(size);

    if (!buffer)
        return false;

    block = XmfBipBuffer_ReadTryReserve(ctx, 0, &blockSize);

    if (block)
    {
        memcpy(&buffer[commitSize], block, blockSize);
        XmfBipBuffer_ReadCommit(ctx, blockSize);
        commitSize += blockSize;
    }

    block = XmfBipBuffer_ReadTryReserve(ctx, 0, &blockSize);

    if (block)
    {
        memcpy(&buffer[commitSize], block, blockSize);
        XmfBipBuffer_ReadCommit(ctx, blockSize);
        commitSize += blockSize;
    }

    XmfBipBuffer_Clear(ctx);

    free(ctx->buffer);
    ctx->buffer = buffer;
    ctx->size = size;

    ctx->blockA.index = 0;
    ctx->blockA.size = commitSize;

    return true;
}

void XmfBipBuffer_FreeBuffer(XmfBipBuffer* ctx)
{
    if (ctx->buffer)
    {
        free(ctx->buffer);
        ctx->buffer = NULL;
    }

    XmfBipBuffer_Clear(ctx);
}

size_t XmfBipBuffer_UsedSize(XmfBipBuffer* ctx)
{
    return ctx->blockA.size + ctx->blockB.size;
}

size_t XmfBipBuffer_BufferSize(XmfBipBuffer* ctx)
{
    return ctx->size;
}

uint8_t* XmfBipBuffer_WriteTryReserve(XmfBipBuffer* ctx, size_t size, size_t* reserved)
{
    size_t reservable;

    if (!reserved)
        return NULL;

    if (!ctx->blockB.size)
    {
        /* block B does not exist */

        reservable = ctx->size - ctx->blockA.index - ctx->blockA.size; /* space after block A */

        if (reservable >= ctx->blockA.index)
        {
            if (reservable == 0)
                return NULL;

            if (size < reservable)
                reservable = size;

            ctx->writeR.size = reservable;
            *reserved = reservable;

            ctx->writeR.index = ctx->blockA.index + ctx->blockA.size;
            return &ctx->buffer[ctx->writeR.index];
        }

        if (ctx->blockA.index == 0)
            return NULL;

        if (ctx->blockA.index < size)
            size = ctx->blockA.index;

        ctx->writeR.size = size;
        *reserved = size;

        ctx->writeR.index = 0;
        return ctx->buffer;
    }

    /* block B exists */

    reservable = ctx->blockA.index - ctx->blockB.index - ctx->blockB.size; /* space after block B */

    if (size < reservable)
        reservable = size;

    if (reservable == 0)
        return NULL;

    ctx->writeR.size = reservable;
    *reserved = reservable;

    ctx->writeR.index = ctx->blockB.index + ctx->blockB.size;
    return &ctx->buffer[ctx->writeR.index];
}

uint8_t* XmfBipBuffer_WriteReserve(XmfBipBuffer* ctx, size_t size)
{
    uint8_t* block = NULL;
    size_t reserved = 0;

    block = XmfBipBuffer_WriteTryReserve(ctx, size, &reserved);

    if (reserved == size)
        return block;

    if (!XmfBipBuffer_Grow(ctx, size))
        return NULL;

    block = XmfBipBuffer_WriteTryReserve(ctx, size, &reserved);

    return block;
}

void XmfBipBuffer_WriteCommit(XmfBipBuffer* ctx, size_t size)
{
    int oldSize = 0;
    int newSize = 0;

    oldSize = (int) XmfBipBuffer_UsedSize(ctx);

    if (size == 0)
    {
        XmfBipBlock_Clear(ctx->writeR);
        goto exit;
    }

    if (size > ctx->writeR.size)
        size = ctx->writeR.size;

    if ((ctx->blockA.size == 0) && (ctx->blockB.size == 0))
    {
        ctx->blockA.index = ctx->writeR.index;
        ctx->blockA.size = size;
        XmfBipBlock_Clear(ctx->writeR);
        goto exit;
    }

    if (ctx->writeR.index == (ctx->blockA.size + ctx->blockA.index))
        ctx->blockA.size += size;
    else
        ctx->blockB.size += size;

    XmfBipBlock_Clear(ctx->writeR);

exit:
    newSize = (int) XmfBipBuffer_UsedSize(ctx);

    if ((oldSize <= 0) && (newSize > 0))
    {
        XmfBipBuffer_SetSignaledState(ctx, true);
    }
}

int XmfBipBuffer_Write(XmfBipBuffer* ctx, const uint8_t* data, size_t size)
{
    int status = 0;
    int oldSize = 0;
    int newSize = 0;
    size_t writeSize = 0;
    size_t blockSize = 0;
    uint8_t* block = NULL;

    if (!ctx)
        return -1;

    oldSize = (int) XmfBipBuffer_UsedSize(ctx);

    block = XmfBipBuffer_WriteReserve(ctx, size);

    if (!block)
    {
        status = -1;
        goto exit;
    }

    block = XmfBipBuffer_WriteTryReserve(ctx, size - status, &blockSize);

    if (block)
    {
        writeSize = size - status;

        if (writeSize > blockSize)
            writeSize = blockSize;

        memcpy(block, &data[status], writeSize);
        XmfBipBuffer_WriteCommit(ctx, writeSize);
        status += (int) writeSize;

        if ((status == size) || (writeSize < blockSize))
            goto exit;
    }

    block = XmfBipBuffer_WriteTryReserve(ctx, size - status, &blockSize);

    if (block)
    {
        writeSize = size - status;

        if (writeSize > blockSize)
            writeSize = blockSize;

        memcpy(block, &data[status], writeSize);
        XmfBipBuffer_WriteCommit(ctx, writeSize);
        status += (int) writeSize;

        if ((status == size) || (writeSize < blockSize))
            goto exit;
    }

exit:
    newSize = (int) XmfBipBuffer_UsedSize(ctx);

    if ((oldSize <= 0) && (newSize > 0))
    {
        XmfBipBuffer_SetSignaledState(ctx, true);
    }

    return status;
}

uint8_t* XmfBipBuffer_ReadTryReserve(XmfBipBuffer* ctx, size_t size, size_t* reserved)
{
    size_t reservable = 0;

    if (!reserved)
        return NULL;

    if (ctx->blockA.size == 0)
    {
        *reserved = 0;
        return NULL;
    }

    reservable = ctx->blockA.size;

    if (size && (reservable > size))
        reservable = size;

    *reserved = reservable;
    return &ctx->buffer[ctx->blockA.index];
}

uint8_t* XmfBipBuffer_ReadReserve(XmfBipBuffer* ctx, size_t size)
{
    uint8_t* block = NULL;
    size_t reserved = 0;

    if (XmfBipBuffer_UsedSize(ctx) < size)
        return NULL;

    block = XmfBipBuffer_ReadTryReserve(ctx, size, &reserved);

    if (reserved == size)
        return block;

    if (!XmfBipBuffer_Grow(ctx, ctx->size + 1))
        return NULL;

    block = XmfBipBuffer_ReadTryReserve(ctx, size, &reserved);

    if (reserved != size)
        return NULL;

    return block;
}

void XmfBipBuffer_ReadCommit(XmfBipBuffer* ctx, size_t size)
{
    int oldSize = 0;
    int newSize = 0;

    if (!ctx)
        return;

    oldSize = (int) XmfBipBuffer_UsedSize(ctx);

    if (size >= ctx->blockA.size)
    {
        XmfBipBlock_Copy(ctx->blockA, ctx->blockB);
        XmfBipBlock_Clear(ctx->blockB);
    }
    else
    {
        ctx->blockA.size -= size;
        ctx->blockA.index += size;
    }

    newSize = (int) XmfBipBuffer_UsedSize(ctx);

    if ((oldSize > 0) && (newSize <= 0))
    {
        XmfBipBuffer_SetSignaledState(ctx, false);
    }
}

int XmfBipBuffer_Read(XmfBipBuffer* ctx, uint8_t* data, size_t size)
{
    int status = 0;
    int oldSize = 0;
    int newSize = 0;
    size_t readSize = 0;
    size_t blockSize = 0;
    uint8_t* block = NULL;

    if (!ctx)
        return -1;

    oldSize = (int) XmfBipBuffer_UsedSize(ctx);

    block = XmfBipBuffer_ReadTryReserve(ctx, 0, &blockSize);

    if (block)
    {
        readSize = size - status;

        if (readSize > blockSize)
            readSize = blockSize;

        memcpy(&data[status], block, readSize);
        XmfBipBuffer_ReadCommit(ctx, readSize);
        status += (int) readSize;

        if ((status == size) || (readSize < blockSize))
            goto exit;
    }

    block = XmfBipBuffer_ReadTryReserve(ctx, 0, &blockSize);

    if (block)
    {
        readSize = size - status;

        if (readSize > blockSize)
            readSize = blockSize;

        memcpy(&data[status], block, readSize);
        XmfBipBuffer_ReadCommit(ctx, readSize);
        status += (int) readSize;

        if ((status == size) || (readSize < blockSize))
            goto exit;
    }

exit:
    newSize = (int) XmfBipBuffer_UsedSize(ctx);

    if ((oldSize > 0) && (newSize <= 0))
    {
        XmfBipBuffer_SetSignaledState(ctx, false);
    }

    return status;
}

bool XmfBipBuffer_GetSignaledState(XmfBipBuffer* ctx)
{
    return ctx->signaled;
}

void XmfBipBuffer_SetSignaledState(XmfBipBuffer* ctx, bool signaled)
{
    ctx->signaled = true;
}

bool XmfBipBuffer_Init(XmfBipBuffer* ctx)
{
    ctx->pageSize = 4096;

    if (!XmfBipBuffer_AllocBuffer(ctx, ctx->size))
        return false;

    return true;
}

void XmfBipBuffer_Uninit(XmfBipBuffer* ctx)
{
    XmfBipBuffer_FreeBuffer(ctx);
}

XmfBipBuffer* XmfBipBuffer_New(size_t size)
{
    XmfBipBuffer* ctx;

    ctx = (XmfBipBuffer*) calloc(1, sizeof(XmfBipBuffer));

    if (!ctx)
        return NULL;

    ctx->size = size;

    if (!XmfBipBuffer_Init(ctx))
    {
        XmfBipBuffer_Free(ctx);
        return NULL;
    }

    return ctx;
}

void XmfBipBuffer_Free(XmfBipBuffer* ctx)
{
    if (!ctx)
        return;

    XmfBipBuffer_Uninit(ctx);

    free(ctx);
}
