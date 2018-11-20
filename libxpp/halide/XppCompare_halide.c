#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <xpp/compare.h>

#include <halide/Copy.h>
#include <halide/Compare32Stage1.h>
#include <halide/Compare8Stage1.h>
#include <HalideRuntime.h>

static void codec_setup_u32_buffer_t(buffer_t* buffer, uint32_t* data, int width, int height, int stride)
{
	memset(buffer, 0, sizeof(buffer_t));

	buffer->extent[0] = width;
	buffer->extent[1] = height;
	buffer->elem_size = 4;
	buffer->host = (uint8_t*)data;
	buffer->stride[0] = 1;
	buffer->stride[1] = stride;
	buffer->host_dirty = true;
}

static void codec_setup_1d_u32_buffer_t(buffer_t* buffer, uint32_t* data, int width)
{
	memset(buffer, 0, sizeof(buffer_t));

	buffer->extent[0] = width;
	buffer->elem_size = 4;
	buffer->host = (uint8_t*)data;
	buffer->stride[0] = 1;
	buffer->host_dirty = true;
}

static void codec_setup_u8_buffer_t(buffer_t* buffer, uint8_t* data, int width, int height, int stride)
{
	memset(buffer, 0, sizeof(buffer_t));

	buffer->extent[0] = width;
	buffer->extent[1] = height;
	buffer->elem_size = 4;
	buffer->host = data;
	buffer->stride[0] = 1;
	buffer->stride[1] = stride;
	buffer->host_dirty = true;
}

static void codec_setup_1d_u8_buffer_t(buffer_t* buffer, uint8_t* data, int width)
{
	memset(buffer, 0, sizeof(buffer_t));

	buffer->extent[0] = width;
	buffer->elem_size = 1;
	buffer->host = data;
	buffer->stride[0] = 1;
	buffer->host_dirty = true;
}

int Xpp_Compare32_halide(uint8_t* pData1, int step1, uint8_t* pData2, int step2, int width, int height,
			      XppRect* rect)
{
	int i;

	buffer_t frame1Buffer;
	buffer_t frame2Buffer;
	buffer_t diffXBuffer;
	buffer_t diffYBuffer;

	uint32_t* diffX = calloc(width, sizeof(uint32_t));
	uint32_t* diffY = calloc(height, sizeof(uint32_t));

	codec_setup_u32_buffer_t(&frame1Buffer, (uint32_t*)pData1, width, height, step1 / sizeof(uint32_t));
	codec_setup_u32_buffer_t(&frame2Buffer, (uint32_t*)pData2, width, height, step2 / sizeof(uint32_t));
	codec_setup_1d_u32_buffer_t(&diffXBuffer, diffX, width);
	codec_setup_1d_u32_buffer_t(&diffYBuffer, diffY, height);

	Compare32Stage1_old_buffer_t(width, height, &frame1Buffer, &frame2Buffer, &diffXBuffer, &diffYBuffer);

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
	int i;

	buffer_t frame1Buffer;
	buffer_t frame2Buffer;
	buffer_t diffXBuffer;
	buffer_t diffYBuffer;

	uint8_t* diffX = calloc(width, 1);
	uint8_t* diffY = calloc(height, 1);

	codec_setup_u8_buffer_t(&frame1Buffer, pData1, width, height, step1);
	codec_setup_u8_buffer_t(&frame2Buffer, pData2, width, height, step2);
	codec_setup_1d_u8_buffer_t(&diffXBuffer, diffX, width);
	codec_setup_1d_u8_buffer_t(&diffYBuffer, diffY, height);

	Compare8Stage1_old_buffer_t(width, height, &frame1Buffer, &frame2Buffer, &diffXBuffer, &diffYBuffer);

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
