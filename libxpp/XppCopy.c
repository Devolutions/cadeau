#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <xpp/math.h>
#include <xpp/color.h>
#include <xpp/codec.h>
#include <xpp/prim.h>

#ifdef WITH_SIMD
#include "simd/simd.h"
#endif

#ifdef WITH_HALIDE
#include <halide/Copy.h>
#include <halide/Compare32Stage1.h>
#include <halide/Compare8Stage1.h>
#include <HalideRuntime.h>
#endif

#ifdef WITH_HALIDE

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

#endif

int Xpp_Copy_c(uint8_t* pDstData, int nDstStep, int nXDst, int nYDst, int nWidth, int nHeight, uint8_t* pSrcData,
		    int nSrcStep, int nXSrc, int nYSrc)
{
	int y;
	uint8_t* pSrcPixel;
	uint8_t* pDstPixel;

	if (nSrcStep < 0)
		nSrcStep = 4 * nWidth;

	if (nDstStep < 0)
		nDstStep = 4 * nWidth;

	pSrcPixel = &pSrcData[(nYSrc * nSrcStep) + (nXSrc * 4)];
	pDstPixel = &pDstData[(nYDst * nDstStep) + (nXDst * 4)];

	for (y = 0; y < nHeight; y++)
	{
		memcpy(pDstPixel, pSrcPixel, nWidth * 4);
		pSrcPixel = &pSrcPixel[nSrcStep];
		pDstPixel = &pDstPixel[nDstStep];
	}

	return 1;
}

int Xpp_Copy(uint8_t* pDstData, int nDstStep, int nXDst, int nYDst, int nWidth, int nHeight, uint8_t* pSrcData,
		  int nSrcStep, int nXSrc, int nYSrc)
{
	XppPrimitives* primitives = XppPrimitives_Get();
	return primitives->Copy(pDstData, nDstStep, nXDst, nYDst, nWidth, nHeight, pSrcData, nSrcStep, nXSrc, nYSrc);
}

int Xpp_Move_c(uint8_t* pData, int nStep, int nXDst, int nYDst, int nWidth, int nHeight, int nXSrc, int nYSrc)
{
	uint8_t* pSrcPixel;
	uint8_t* pDstPixel;

	if (nStep < 0)
		nStep = 4 * nWidth;

	pSrcPixel = &pData[((nYSrc + nHeight - 1) * nStep) + (nXSrc * 4)];
	pDstPixel = &pData[((nYDst + nHeight - 1) * nStep) + (nXDst * 4)];

	if (pSrcPixel > pDstPixel)
		return Xpp_Copy_c(pData, nStep, nXDst, nYDst, nWidth, nHeight, pData,
			nStep, nXSrc, nYSrc);

	while (nHeight--)
	{
		memmove(pDstPixel, pSrcPixel, nWidth * 4);
		pSrcPixel -= nStep;
		pDstPixel -= nStep;
	}

	return 1;
}

int Xpp_Move(uint8_t* pData, int nStep, int nXDst, int nYDst, int nWidth, int nHeight, int nXSrc, int nYSrc)
{
	XppPrimitives* primitives = XppPrimitives_Get();
	return primitives->Move(pData, nStep, nXDst, nYDst, nWidth, nHeight, nXSrc, nYSrc);
}

int Xpp_CopyFromRetina_c(uint8_t* pDstData, int nDstStep, int nXDst, int nYDst, int nWidth, int nHeight,
			      uint8_t* pSrcData, int nSrcStep, int nXSrc, int nYSrc)
{
	int x, y;
	int nSrcPad;
	int nDstPad;
	uint32_t R, G, B;
	uint8_t* pSrcPixel;
	uint8_t* pDstPixel;

	if (nSrcStep < 0)
		nSrcStep = 8 * nWidth;

	if (nDstStep < 0)
		nDstStep = 4 * nWidth;

	nSrcPad = (nSrcStep - (nWidth * 8));
	nDstPad = (nDstStep - (nWidth * 4));

	pSrcPixel = &pSrcData[(nYSrc * nSrcStep) + (nXSrc * 8)];
	pDstPixel = &pDstData[(nYDst * nDstStep) + (nXDst * 4)];

	for (y = 0; y < nHeight; y++)
	{
		for (x = 0; x < nWidth; x++)
		{
			/* simple box filter scaling, could be improved with better algorithm */

			B = pSrcPixel[0] + pSrcPixel[4] + pSrcPixel[nSrcStep + 0] + pSrcPixel[nSrcStep + 4];
			G = pSrcPixel[1] + pSrcPixel[5] + pSrcPixel[nSrcStep + 1] + pSrcPixel[nSrcStep + 5];
			R = pSrcPixel[2] + pSrcPixel[6] + pSrcPixel[nSrcStep + 2] + pSrcPixel[nSrcStep + 6];
			pSrcPixel += 8;

			*pDstPixel++ = (uint8_t) (B >> 2);
			*pDstPixel++ = (uint8_t) (G >> 2);
			*pDstPixel++ = (uint8_t) (R >> 2);
			*pDstPixel++ = 0xFF;
		}

		pSrcPixel = &pSrcPixel[nSrcPad + nSrcStep];
		pDstPixel = &pDstPixel[nDstPad];
	}

	return 1;
}

int Xpp_CopyFromRetina(uint8_t* pDstData, int nDstStep, int nXDst, int nYDst, int nWidth, int nHeight,
			    uint8_t* pSrcData, int nSrcStep, int nXSrc, int nYSrc)
{
	XppPrimitives* primitives = XppPrimitives_Get();
	return primitives->CopyFromRetina(pDstData, nDstStep, nXDst, nYDst, nWidth, nHeight, pSrcData, nSrcStep, nXSrc,
					  nYSrc);
}
