#include "XmfPng.h"

#include "XmfFile.h"

#include <libpng16/png.h>

bool XmfPng_SaveFile(const char* filename, const uint8_t* data, uint32_t width, uint32_t height, uint32_t step)
{
    int png_status;
    png_image image;
    bool success = false;

    memset(&image, 0, sizeof(png_image));
    image.version = PNG_IMAGE_VERSION;
    image.format = PNG_FORMAT_BGRA;
    image.width = width;
    image.height = height;

    png_status = png_image_write_to_file(&image, filename, 0,
        (const void*)data, (png_int_32)step, NULL);

    if (!png_status)
        goto exit;

    success = true;
exit:
    png_image_free(&image);
    return success;
}

bool XmfPng_LoadData(const uint8_t* fileData, size_t fileSize, uint8_t** data, uint32_t* width, uint32_t* height, uint32_t* step)
{
    int png_status;
    int row_stride;
    png_image image;
    bool success = false;
    uint8_t* buffer = NULL;

    memset(&image, 0, sizeof(png_image));
    image.version = PNG_IMAGE_VERSION;

    png_status = png_image_begin_read_from_memory(&image, (png_const_voidp)fileData, (png_size_t)fileSize);

    if (!png_status)
        goto exit;

    image.format = PNG_FORMAT_BGRA;
    row_stride = PNG_IMAGE_ROW_STRIDE(image);

    buffer = malloc(PNG_IMAGE_SIZE(image));

    if (!buffer)
        goto exit;

    png_status = png_image_finish_read(&image, NULL, buffer, row_stride, NULL);

    if (!png_status)
        goto exit;

    *data = buffer;
    *width = image.width;
    *height = image.height;

    if (step)
        *step = (uint32_t) row_stride;

    success = true;
exit:
    if (!success)
        free(buffer);

    png_image_free(&image);
    return success;
}

bool XmfPng_LoadFile(const char* filename, uint8_t** data, uint32_t* width, uint32_t* height, uint32_t* step)
{
    bool success = false;
    uint8_t* fileData = NULL;
    size_t fileSize = 0;

    fileData = XmfFile_Load(filename, &fileSize, 0);

    if (!fileData)
        return false;

    if (!XmfPng_LoadData(fileData, fileSize, data, width, height, step))
    {
        goto exit;
    }

    success = true;
exit:
    free(fileData);
    return success;
}
