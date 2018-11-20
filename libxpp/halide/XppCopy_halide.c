#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <xpp/copy.h>

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

int Xpp_Copy_halide(uint8_t* pDstData, int nDstStep, int nXDst, int nYDst,
	int nWidth, int nHeight, uint8_t* pSrcData, int nSrcStep, int nXSrc, int nYSrc)
{
	buffer_t input;
	buffer_t output;

	codec_setup_u32_buffer_t(&input, (uint32_t*)&pSrcData[nXSrc * sizeof(uint32_t) + nYSrc * nSrcStep], nWidth,
				 nHeight, nSrcStep / sizeof(uint32_t));
	codec_setup_u32_buffer_t(&output, (uint32_t*)&pDstData[nXDst * sizeof(uint32_t) + nYDst * nDstStep], nWidth,
				 nHeight, nDstStep / sizeof(uint32_t));

	Copy_old_buffer_t(&input, &output);

	return 0;
}
