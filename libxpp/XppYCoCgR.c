/* 16-bit signed YCoCg-R */

void Xpp_RGBToYCoCgR_16s_P3AC4R(const uint8_t* pSrc, int32_t srcStep,
	int16_t* pDst[3], int32_t dstStep[3], uint32_t width, uint32_t height)
{
	int x, y;
	int srcPad;
	int dstPad[3];
	int16_t* pY;
	int16_t* pCo;
	int16_t* pCg;
	int32_t R, G, B;
	int32_t Y, Co, Cg, t;
	const uint8_t* pRGB = pSrc;

	pY = pDst[0];
	pCo = pDst[1];
	pCg = pDst[2];

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

			Co = R - B;
			t = B + (Co >> 1);
			Cg = G - t;
			Y = t + (Cg >> 1);

			Y -= 128;

			/**
			 * Y:  [-128, 127]
			 * Co: [-256, 255]
			 * Cg: [-256, 255]
			 */

			*pY = (int16_t) Y;
			*pCo = (int16_t) Co;
			*pCg = (int16_t) Cg;

			pY++;
			pCo++;
			pCg++;
		}

		pRGB += srcPad;
		pY = (int16_t*) (((uint8_t*) pY) + dstPad[0]);
		pCo = (int16_t*) (((uint8_t*) pCo) + dstPad[1]);
		pCg = (int16_t*) (((uint8_t*) pCg) + dstPad[2]);
	}
}

void Xpp_YCoCgRToRGB_16s_P3AC4R(const int16_t* pSrc[3], uint32_t srcStep[3],
				uint8_t* pDst, uint32_t dstStep, uint32_t width, uint32_t height)
{
	int x, y;
	int dstPad;
	int srcPad[3];
	const int16_t* pY;
	const int16_t* pCo;
	const int16_t* pCg;
	int32_t R, G, B;
	int32_t Y, Co, Cg, t;
	uint8_t* pRGB = pDst;

	pY = pSrc[0];
	pCo = pSrc[1];
	pCg = pSrc[2];

	dstPad = (dstStep - (width * 4));
	srcPad[0] = (srcStep[0] - (width * 2));
	srcPad[1] = (srcStep[1] - (width * 2));
	srcPad[2] = (srcStep[2] - (width * 2));

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			Y = *pY;
			Co = *pCo;
			Cg = *pCg;

			pY++;
			pCo++;
			pCg++;

			Y += 128;

			t = Y - (Cg >> 1);
			G = Cg + t;
			B = t - (Co >> 1);
			R = B + Co;

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
		pCo = (int16_t*) (((uint8_t*) pCo) + srcPad[1]);
		pCg = (int16_t*) (((uint8_t*) pCg) + srcPad[2]);
	}
}

