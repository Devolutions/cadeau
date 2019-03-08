#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <xpp/color.h>

#include <halide/YCoCgR420ToRgb.h>
#include <halide/RgbToYCoCgR420.h>
#include <HalideRuntime.h>

#include "XppHalide.c"

void Xpp_Halide_RGBToYCoCgR420_8u_P3AC4R(const uint8_t* pSrc, int32_t srcStep, uint8_t* pDst[3],
					      int32_t dstStep[3], int width, int height)
{
	halide_buffer_t inRgb;
	halide_buffer_t outY;
	halide_buffer_t outCo;
	halide_buffer_t outCg;

	halide_setup_rgb_buffer_t(&inRgb, (uint8_t*) pSrc, width, height, srcStep);
	halide_setup_ycocg_buffer_t(&outY, (uint8_t*) pDst[0], width, height, dstStep[0]);
	halide_setup_ycocg_buffer_t(&outCo, (uint8_t*) pDst[1], width / 2, height / 2, dstStep[1]);
	halide_setup_ycocg_buffer_t(&outCg, (uint8_t*) pDst[2], width / 2, height / 2, dstStep[2]);
	
	RgbToYCoCgR420(&inRgb, &outY, &outCo, &outCg);
}

void Xpp_Halide_YCoCgR420ToRGB_8u_P3AC4R(const uint8_t* pSrc[3], int srcStep[3], uint8_t* pDst, int dstStep,
					      int width, int height)
{
	halide_buffer_t outRgb;
	halide_buffer_t inY;
	halide_buffer_t inCo;
	halide_buffer_t inCg;

	halide_setup_rgb_buffer_t(&outRgb, (uint8_t*) pDst, width, height, dstStep);
	halide_setup_ycocg_buffer_t(&inY, (uint8_t*) pSrc[0], width, height, srcStep[0]);
	halide_setup_ycocg_buffer_t(&inCo, (uint8_t*) pSrc[1], width / 2, height / 2, srcStep[1]);
	halide_setup_ycocg_buffer_t(&inCg, (uint8_t*) pSrc[2], width / 2, height / 2, srcStep[2]);

	YCoCgR420ToRgb(&inY, &inCo, &inCg, &outRgb);
}
