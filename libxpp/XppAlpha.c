
void Xpp_MultiplyAlpha(const uint8_t* pSrc, int32_t srcStep, uint8_t* pDst, int32_t dstStep, int width, int height)
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
}

void Xpp_UnmultiplyAlpha(const uint8_t* pSrc, int32_t srcStep, uint8_t* pDst, int32_t dstStep, int width,
			 int height)
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
}
