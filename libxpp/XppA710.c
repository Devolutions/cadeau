#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <xpp/math.h>
#include <xpp/primitive.h>

#include <xpp/color.h>

#define CLAMP(_val, _min, _max) \
	if (_val < _min) _val = _min; \
	else if (_val > _max) _val = _max;

/* 16-bit signed A710 */

XppStatus Xpp_RGBToA710_16s_P3AC4R(const uint8_t* pSrc, uint32_t srcStep,
			       int16_t* pDst[3], uint32_t dstStep[3], uint32_t width, uint32_t height)
{
	int x, y;
	int srcPad;
	int dstPad[3];
	int16_t* pY;
	int16_t* pU;
	int16_t* pV;
	int32_t R, G, B;
	int32_t Y, U, V, t;
	const uint8_t* pRGB = pSrc;

	pY = pDst[0];
	pU = pDst[1];
	pV = pDst[2];

	srcPad = (srcStep - (width * 4));
	dstPad[0] = (dstStep[0] - (width * 2));
	dstPad[1] = (dstStep[1] - (width * 2));
	dstPad[2] = (dstStep[2] - (width * 2));

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			B = pRGB[0];
			G = pRGB[1];
			R = pRGB[2];
			pRGB += 4;

			/*
			 * V = R - G
			 * t = B - G
			 * Y = G + ((V + t) >> 2)
			 * U = t - (V >> 1)
			 */

			t = B - G;
			V = R - G;
			Y = G + ((V + t) >> 2);
			U = t - (V >> 1);

			Y -= 128;

			/**
			 * Y: [-128, 127]
			 * U: [-256, 255]
			 * V: [-256, 255]
			 */

			*pY = (int16_t) Y;
			*pU = (int16_t) U;
			*pV = (int16_t) V;

			pY++;
			pU++;
			pV++;
		}

		pRGB += srcPad;
		pY = (int16_t*) (((uint8_t*) pY) + dstPad[0]);
		pU = (int16_t*) (((uint8_t*) pU) + dstPad[1]);
		pV = (int16_t*) (((uint8_t*) pV) + dstPad[2]);
	}

	return XppSuccess;
}

XppStatus Xpp_A710ToRGB_16s_P3AC4R(const int16_t* pSrc[3], uint32_t srcStep[3],
			       uint8_t* pDst, uint32_t dstStep, uint32_t width, uint32_t height)
{
	int x, y;
	int dstPad;
	int srcPad[3];
	const int16_t* pY;
	const int16_t* pU;
	const int16_t* pV;
	int32_t R, G, B;
	int32_t Y, U, V, t;
	uint8_t* pRGB = pDst;

	pY = pSrc[0];
	pU = pSrc[1];
	pV = pSrc[2];

	dstPad = (dstStep - (width * 4));
	srcPad[0] = (srcStep[0] - (width * 2));
	srcPad[1] = (srcStep[1] - (width * 2));
	srcPad[2] = (srcStep[2] - (width * 2));

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			Y = *pY;
			U = *pU;
			V = *pV;

			pY++;
			pU++;
			pV++;

			Y += 128;

			/**
			 * t = U + (V >> 1)
			 * G = Y - ((V + t) >> 2))
			 * B = t + G
			 * R = V + G
			 */

			t = U + (V >> 1);
			G = Y - ((V + t) >> 2);
			B = t + G;
			R = V + G;

			CLAMP(R, 0, 255);
			CLAMP(G, 0, 255);
			CLAMP(B, 0, 255);

			pRGB[0] = (uint8_t) B;
			pRGB[1] = (uint8_t) G;
			pRGB[2] = (uint8_t) R;
			pRGB[3] = 0xFF;
			pRGB += 4;
		}

		pRGB += dstPad;
		pY = (int16_t*) (((uint8_t*) pY) + srcPad[0]);
		pU = (int16_t*) (((uint8_t*) pU) + srcPad[1]);
		pV = (int16_t*) (((uint8_t*) pV) + srcPad[2]);
	}

	return XppSuccess;
}
