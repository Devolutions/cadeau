#include "XmfFile.h"

#include "XmfBmp.h"

#pragma pack(push, 1)

struct _XMF_BITMAP_FILE_HEADER
{
    uint8_t bfType[2];
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
};
typedef struct _XMF_BITMAP_FILE_HEADER XMF_BITMAP_FILE_HEADER;

struct _XMF_BITMAP_INFO_HEADER
{
    uint32_t biSize;
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter;
    int32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};
typedef struct _XMF_BITMAP_INFO_HEADER XMF_BITMAP_INFO_HEADER;

#pragma pack(pop)

bool XmfBmp_SaveFile(const char* filename, const uint8_t* data, uint32_t width, uint32_t height, uint32_t step)
{
    FILE* fp;
    bool success = true;
    XMF_BITMAP_FILE_HEADER bf;
    XMF_BITMAP_INFO_HEADER bi;

    fp = XmfFile_Open(filename, "wb");

    if (!fp)
        return false;

    bf.bfType[0] = 'B';
    bf.bfType[1] = 'M';
    bf.bfReserved1 = 0;
    bf.bfReserved2 = 0;
    bf.bfOffBits = sizeof(XMF_BITMAP_FILE_HEADER) + sizeof(XMF_BITMAP_INFO_HEADER);
    bi.biSizeImage = height * step;
    bf.bfSize = bf.bfOffBits + bi.biSizeImage;

    bi.biWidth = width;
    bi.biHeight = -1 * (int32_t) height; // top-down scanline order requires a negative height
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;
    bi.biSize = sizeof(XMF_BITMAP_INFO_HEADER);

    if (fwrite(&bf, sizeof(XMF_BITMAP_FILE_HEADER), 1, fp) != 1 ||
        fwrite(&bi, sizeof(XMF_BITMAP_INFO_HEADER), 1, fp) != 1 ||
        fwrite(data, bi.biSizeImage, 1, fp) != 1)
    {
        success = false;
    }

    XmfFile_Close(fp);

    return success;
}

bool XmfBmp_LoadFile(const char* filename, uint8_t** data, uint32_t* width, uint32_t* height, uint32_t* step)
{
    FILE* fp;
    size_t size = 0;
    uint8_t* buffer = NULL;
    bool success = false;
    XMF_BITMAP_FILE_HEADER bf;
    XMF_BITMAP_INFO_HEADER bi;

    fp = XmfFile_Open(filename, "rb");

    if (!fp)
        goto exit;

    if (fread(&bf, sizeof(XMF_BITMAP_FILE_HEADER), 1, fp) != 1)
        goto exit;

    if (fread(&bi, sizeof(XMF_BITMAP_INFO_HEADER), 1, fp) != 1)
        goto exit;

    if (bi.biCompression != 0)
        goto exit; // bitmap compression not supported

    if (bi.biBitCount != 32)
        goto exit; // bitmap non-XRGB images not supported

    if (bi.biHeight >= 0)
        goto exit; // bitmap bottom-up images not supported

    size = (size_t) bi.biSizeImage;
    buffer = (uint8_t*) malloc(bi.biSizeImage);

    if (!buffer)
        goto exit;

    if (fread(buffer, 1, size, fp) != size)
    {
        free(buffer);
        goto exit;
    }

    *data = buffer;
    *width = (uint32_t) bi.biWidth;
    *height = (uint32_t) (-1 * bi.biHeight);
    *step = (uint32_t) (bi.biSizeImage / *height);

    success = true;
exit:
    if (fp) {
        fclose(fp);
    }
    return success;
}
