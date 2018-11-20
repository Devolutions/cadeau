#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <xpp/color.h>

#include <halide/YCoCgR420ToRgb.h>
#include <halide/RgbToYCoCgR420.h>
#include <HalideRuntime.h>

void setup_rgb_buffer_t(buffer_t* buffer, uint8_t* data, int width, int height, int stride)
{
	memset(buffer, 0, sizeof(buffer_t));
	
	buffer->extent[0] = 4;
	buffer->extent[1] = width;
	buffer->extent[2] = height;
	buffer->elem_size = 1;
	buffer->host = data;
	buffer->stride[0] = 1;
	buffer->stride[1] = 4;
	buffer->stride[2] = stride;
	buffer->host_dirty = true;
}

void setup_ycocg_buffer_t(buffer_t* buffer, uint8_t* data, int width, int height, int stride)
{
	memset(buffer, 0, sizeof(buffer_t));
	
	buffer->extent[0] = width;
	buffer->extent[1] = height;
	buffer->elem_size = 1;
	buffer->host = data;
	buffer->stride[0] = 1;
	buffer->stride[1] = stride;
	buffer->host_dirty = true;
}

void Xpp_Halide_RGBToYCoCgR420_8u_P3AC4R(const uint8_t* pSrc, int32_t srcStep, uint8_t* pDst[3],
					      int32_t dstStep[3], int width, int height)
{
	buffer_t inRgb;
	buffer_t outY;
	buffer_t outCo;
	buffer_t outCg;
	
	setup_rgb_buffer_t(&inRgb, (uint8_t*) pSrc, width, height, srcStep);
	setup_ycocg_buffer_t(&outY, (uint8_t*) pDst[0], width, height, dstStep[0]);
	setup_ycocg_buffer_t(&outCo, (uint8_t*) pDst[1], width / 2, height / 2, dstStep[1]);
	setup_ycocg_buffer_t(&outCg, (uint8_t*) pDst[2], width / 2, height / 2, dstStep[2]);
	
	RgbToYCoCgR420_old_buffer_t(&inRgb, &outY, &outCo, &outCg);
}

void Xpp_Halide_YCoCgR420ToRGB_8u_P3AC4R(const uint8_t* pSrc[3], int srcStep[3], uint8_t* pDst, int dstStep,
					      int width, int height)
{
	buffer_t outRgb;
	buffer_t inY;
	buffer_t inCo;
	buffer_t inCg;

	setup_rgb_buffer_t(&outRgb, (uint8_t*) pDst, width, height, dstStep);
	setup_ycocg_buffer_t(&inY, (uint8_t*) pSrc[0], width, height, srcStep[0]);
	setup_ycocg_buffer_t(&inCo, (uint8_t*) pSrc[1], width / 2, height / 2, srcStep[1]);
	setup_ycocg_buffer_t(&inCg, (uint8_t*) pSrc[2], width / 2, height / 2, srcStep[2]);

	YCoCgR420ToRgb_old_buffer_t(&inY, &inCo, &inCg, &outRgb);
}
