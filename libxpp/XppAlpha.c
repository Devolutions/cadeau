#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <xpp/math.h>
#include <xpp/primitive.h>

#include <xpp/color.h>

XppStatus Xpp_MultiplyAlpha(const uint8_t* pSrc, uint32_t srcStep,
	uint8_t* pDst, uint32_t dstStep, uint32_t width, uint32_t height)
{
	int x, y;
	int srcPad;
	int dstPad;

	srcPad = (srcStep - (width * 4));
	dstPad = (srcStep - (width * 4));

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pDst[0] = ((pSrc[0] * pSrc[3]) / 255);
			pDst[1] = ((pSrc[1] * pSrc[3]) / 255);
			pDst[2] = ((pSrc[2] * pSrc[3]) / 255);
			pDst[3] = pSrc[3];

			pSrc += 4;
			pDst += 4;
		}

		pSrc += srcPad;
		pDst += dstPad;
	}

	return XppSuccess;
}

XppStatus Xpp_UnmultiplyAlpha(const uint8_t* pSrc, uint32_t srcStep,
	uint8_t* pDst, uint32_t dstStep, uint32_t width, uint32_t height)
{
	int x, y;
	int srcPad;
	int dstPad;

	srcPad = (srcStep - (width * 4));
	dstPad = (srcStep - (width * 4));

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pDst[0] = (pSrc[0] * 255) / pSrc[3];
			pDst[1] = (pSrc[1] * 255) / pSrc[3];
			pDst[2] = (pSrc[2] * 255) / pSrc[3];
			pDst[3] = pSrc[3];

			pSrc += 4;
			pDst += 4;
		}

		pSrc += srcPad;
		pDst += dstPad;
	}

	return XppSuccess;
}
