#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <xpp/color.h>

#include <halide/RgbToYCoCgR.h>
#include <halide/YCoCgRToRgb.h>
#include <halide/RgbToYCoCgR420.h>
#include <halide/YCoCgR420ToRgb.h>

#include "XppHalide.h"

XppStatus Xpp_RGBToYCoCgR_16s_P3AC4R_halide(const uint8_t* pSrc, uint32_t srcStep,
                         int16_t* pDst[3], uint32_t dstStep[3], uint32_t width, uint32_t height)
{
    HALIDE_BUFFER_DEFINE(inRgb);
    HALIDE_BUFFER_DEFINE(outY);
    HALIDE_BUFFER_DEFINE(outCo);
    HALIDE_BUFFER_DEFINE(outCg);

    halide_setup_rgb_buffer_t(&inRgb, (uint8_t*) pSrc, width, height, srcStep);
    halide_setup_16s_buffer_t(&outY, (uint8_t*) pDst[0], width, height, dstStep[0] / 2);
    halide_setup_16s_buffer_t(&outCo, (uint8_t*) pDst[1], width, height, dstStep[1] / 2);
    halide_setup_16s_buffer_t(&outCg, (uint8_t*) pDst[2], width, height, dstStep[2] / 2);

    RgbToYCoCgR(&inRgb, &outY, &outCo, &outCg);

    return XppSuccess;
}

XppStatus Xpp_YCoCgRToRGB_16s_P3AC4R_halide(const int16_t* pSrc[3], uint32_t srcStep[3],
                         uint8_t* pDst, uint32_t dstStep, uint32_t width, uint32_t height)
{
    HALIDE_BUFFER_DEFINE(outRgb);
    HALIDE_BUFFER_DEFINE(inY);
    HALIDE_BUFFER_DEFINE(inCo);
    HALIDE_BUFFER_DEFINE(inCg);

    halide_setup_rgb_buffer_t(&outRgb, (uint8_t*) pDst, width, height, dstStep);
    halide_setup_16s_buffer_t(&inY, (uint8_t*) pSrc[0], width, height, srcStep[0] / 2);
    halide_setup_16s_buffer_t(&inCo, (uint8_t*) pSrc[1], width, height, srcStep[1] / 2);
    halide_setup_16s_buffer_t(&inCg, (uint8_t*) pSrc[2], width, height, srcStep[2] / 2);

    YCoCgRToRgb(&inY, &inCo, &inCg, &outRgb);

    return XppSuccess;
}

XppStatus Xpp_RGBToYCoCgR420_8u_P3AC4R_halide(const uint8_t* pSrc, uint32_t srcStep, uint8_t* pDst[3],
                          uint32_t dstStep[3], uint32_t width, uint32_t height)
{
    HALIDE_BUFFER_DEFINE(inRgb);
    HALIDE_BUFFER_DEFINE(outY);
    HALIDE_BUFFER_DEFINE(outCo);
    HALIDE_BUFFER_DEFINE(outCg);

    halide_setup_rgb_buffer_t(&inRgb, (uint8_t*) pSrc, width, height, srcStep);
    halide_setup_ycocg_buffer_t(&outY, (uint8_t*) pDst[0], width, height, dstStep[0]);
    halide_setup_ycocg_buffer_t(&outCo, (uint8_t*) pDst[1], width / 2, height / 2, dstStep[1]);
    halide_setup_ycocg_buffer_t(&outCg, (uint8_t*) pDst[2], width / 2, height / 2, dstStep[2]);
    
    RgbToYCoCgR420(&inRgb, &outY, &outCo, &outCg);

    return XppSuccess;
}

XppStatus Xpp_YCoCgR420ToRGB_8u_P3AC4R_halide(const uint8_t* pSrc[3], uint32_t srcStep[3], uint8_t* pDst, uint32_t dstStep,
                     uint32_t width, uint32_t height)
{
    HALIDE_BUFFER_DEFINE(outRgb);
    HALIDE_BUFFER_DEFINE(inY);
    HALIDE_BUFFER_DEFINE(inCo);
    HALIDE_BUFFER_DEFINE(inCg);

    halide_setup_rgb_buffer_t(&outRgb, (uint8_t*) pDst, width, height, dstStep);
    halide_setup_ycocg_buffer_t(&inY, (uint8_t*) pSrc[0], width, height, srcStep[0]);
    halide_setup_ycocg_buffer_t(&inCo, (uint8_t*) pSrc[1], width / 2, height / 2, srcStep[1]);
    halide_setup_ycocg_buffer_t(&inCg, (uint8_t*) pSrc[2], width / 2, height / 2, srcStep[2]);

    YCoCgR420ToRgb(&inY, &inCo, &inCg, &outRgb);

    return XppSuccess;
}
