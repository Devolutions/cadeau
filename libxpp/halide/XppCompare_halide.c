#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <xpp/compare.h>

#include <halide/Copy.h>
#include <halide/Compare32Stage1.h>
#include <halide/Compare8Stage1.h>
#include <HalideRuntime.h>

#include "XppHalide.c"

int Xpp_Compare32_halide(uint8_t* pData1, int step1, uint8_t* pData2, int step2, int width, int height,
			      XppRect* rect)
{
	halide_buffer_t frame1Buffer;
	halide_buffer_t frame2Buffer;
	halide_buffer_t diffXBuffer;
	halide_buffer_t diffYBuffer;

	int i;
	uint32_t* diffX = calloc(width, sizeof(uint32_t));
	uint32_t* diffY = calloc(height, sizeof(uint32_t));

	halide_setup_u32_buffer_t(&frame1Buffer, (uint32_t*) pData1, width, height, step1 / sizeof(uint32_t));
	halide_setup_u32_buffer_t(&frame2Buffer, (uint32_t*) pData2, width, height, step2 / sizeof(uint32_t));
	halide_setup_1d_u32_buffer_t(&diffXBuffer, diffX, width);
	halide_setup_1d_u32_buffer_t(&diffYBuffer, diffY, height);

	Compare32Stage1(width, height, &frame1Buffer, &frame2Buffer, &diffXBuffer, &diffYBuffer);

	rect->left = width - 1;
	rect->bottom = height - 1;

	for (i = 0; i < width; i++)
	{
		if (diffX[0] == 0) rect->left = min(rect->left, i);
		if (diffX[0] != 0) rect->right = max(rect->right, i);
	}

	for (i = 0; i < width; i++)
	{
		if (diffY[0] == 0) rect->top = min(rect->top, i);
		if (diffY[0] != 0) rect->bottom = max(rect->bottom, i);
	}

	free(diffX);
	free(diffY);

	return 0;
}

int Xpp_Compare8_halide(uint8_t* pData1, int step1, uint8_t* pData2, int step2, int width, int height,
			     XppRect* rect)
{
	halide_buffer_t frame1Buffer;
	halide_buffer_t frame2Buffer;
	halide_buffer_t diffXBuffer;
	halide_buffer_t diffYBuffer;

	int i;
	uint8_t* diffX = calloc(width, 1);
	uint8_t* diffY = calloc(height, 1);

	halide_setup_u8_buffer_t(&frame1Buffer, pData1, width, height, step1);
	halide_setup_u8_buffer_t(&frame2Buffer, pData2, width, height, step2);
	halide_setup_1d_u8_buffer_t(&diffXBuffer, diffX, width);
	halide_setup_1d_u8_buffer_t(&diffYBuffer, diffY, height);

	Compare8Stage1(width, height, &frame1Buffer, &frame2Buffer, &diffXBuffer, &diffYBuffer);

	rect->left = width - 1;
	rect->bottom = height - 1;

	for (i = 0; i < width; i++)
	{
		if (diffX[0] == 0) rect->left = min(rect->left, i);
		if (diffX[0] != 0) rect->right = max(rect->right, i);
	}

	for (i = 0; i < width; i++)
	{
		if (diffY[0] == 0) rect->top = min(rect->top, i);
		if (diffY[0] != 0) rect->bottom = max(rect->bottom, i);
	}

	free(diffX);
	free(diffY);

	return 0;
}
