#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <xpp/math.h>

#include <xpp/compare.h>

#include <halide/Copy.h>
#include <halide/Compare32Stage1.h>
#include <halide/Compare8Stage1.h>
#include <HalideRuntime.h>

#include "XppHalide.h"

XppStatus Xpp_Compare32_halide(uint8_t* pData1, int32_t step1, uint8_t* pData2, int32_t step2,
            int32_t width, int32_t height, XppRect* rect)
{
    HALIDE_BUFFER_DEFINE(frame1Buffer);
    HALIDE_BUFFER_DEFINE(frame2Buffer);
    HALIDE_BUFFER_DEFINE(diffXBuffer);
    HALIDE_BUFFER_DEFINE(diffYBuffer);

    int32_t i;
    uint32_t* diffX = xpp_calloc(width, sizeof(uint32_t));
    uint32_t* diffY = xpp_calloc(height, sizeof(uint32_t));

    halide_setup_u32_buffer_t(&frame1Buffer, (uint32_t*) pData1, width, height, step1 / sizeof(uint32_t));
    halide_setup_u32_buffer_t(&frame2Buffer, (uint32_t*) pData2, width, height, step2 / sizeof(uint32_t));
    halide_setup_1d_u32_buffer_t(&diffXBuffer, diffX, width);
    halide_setup_1d_u32_buffer_t(&diffYBuffer, diffY, height);

    Compare32Stage1(width, height, &frame1Buffer, &frame2Buffer, &diffXBuffer, &diffYBuffer);

    rect->left = width - 1;
    rect->bottom = height - 1;

    for (i = 0; i < width; i++)
    {
        if (diffX[0] == 0)
            rect->left = min(rect->left, i);

        if (diffX[0] != 0)
            rect->right = max(rect->right, i);
    }

    for (i = 0; i < height; i++)
    {
        if (diffY[0] == 0)
            rect->top = min(rect->top, i);

        if (diffY[0] != 0)
            rect->bottom = max(rect->bottom, i);
    }

    xpp_free(diffX);
    xpp_free(diffY);

    return 1;
}

XppStatus Xpp_Compare8_halide(uint8_t* pData1, int32_t step1, uint8_t* pData2, int32_t step2,
    int32_t width, int32_t height, XppRect* rect)
{
    HALIDE_BUFFER_DEFINE(frame1Buffer);
    HALIDE_BUFFER_DEFINE(frame2Buffer);
    HALIDE_BUFFER_DEFINE(diffXBuffer);
    HALIDE_BUFFER_DEFINE(diffYBuffer);

    int32_t i;
    uint8_t* diffX = xpp_calloc(width, sizeof(uint8_t));
    uint8_t* diffY = xpp_calloc(height, sizeof(uint8_t));

    halide_setup_u8_buffer_t(&frame1Buffer, pData1, width, height, step1);
    halide_setup_u8_buffer_t(&frame2Buffer, pData2, width, height, step2);
    halide_setup_1d_u8_buffer_t(&diffXBuffer, diffX, width);
    halide_setup_1d_u8_buffer_t(&diffYBuffer, diffY, height);

    Compare8Stage1(width, height, &frame1Buffer, &frame2Buffer, &diffXBuffer, &diffYBuffer);

    rect->left = width - 1;
    rect->bottom = height - 1;

    for (i = 0; i < width; i++)
    {
        if (diffX[0] == 0)
            rect->left = min(rect->left, i);

        if (diffX[0] != 0)
            rect->right = max(rect->right, i);
    }

    for (i = 0; i < height; i++)
    {
        if (diffY[0] == 0)
            rect->top = min(rect->top, i);

        if (diffY[0] != 0)
            rect->bottom = max(rect->bottom, i);
    }

    xpp_free(diffX);
    xpp_free(diffY);

    return 1;
}
