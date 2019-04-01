
#include <xpp/copy.h>
#include <xpp/compare.h>

#define XPP_SIMD_INTERNAL
#include "XppSimd.h"
#include "emmintrin.h"

#if !defined(_MSC_VER)
#define _mm_castsi128_ps(a) ((__m128)(a))
#define _mm_castps_si128(a) ((__m128i)(a))
#endif

#define COPY_LOAD(reg, ptr, load_fct)  \
{  \
	reg = load_fct((__m128i *)(ptr));  \
	ptr += 16;  \
}

#define COPY_STORE(reg, ptr, store_fct)  \
{  \
	store_fct((__m128i *)(ptr), reg);  \
	ptr += 16;  \
}

#define COPY32(load_fct, store_fct)  \
{  \
	COPY_LOAD(xmm0, pSrcPixel, load_fct);  \
	COPY_LOAD(xmm1, pSrcPixel, load_fct);  \
	COPY_LOAD(xmm2, pSrcPixel, load_fct);  \
	COPY_LOAD(xmm3, pSrcPixel, load_fct);  \
	COPY_LOAD(xmm4, pSrcPixel, load_fct);  \
	COPY_LOAD(xmm5, pSrcPixel, load_fct);  \
	COPY_LOAD(xmm6, pSrcPixel, load_fct);  \
	COPY_LOAD(xmm7, pSrcPixel, load_fct);  \
	COPY_STORE(xmm0, pDstPixel, store_fct);  \
	COPY_STORE(xmm1, pDstPixel, store_fct);  \
	COPY_STORE(xmm2, pDstPixel, store_fct);  \
	COPY_STORE(xmm3, pDstPixel, store_fct);  \
	COPY_STORE(xmm4, pDstPixel, store_fct);  \
	COPY_STORE(xmm5, pDstPixel, store_fct);  \
	COPY_STORE(xmm6, pDstPixel, store_fct);  \
	COPY_STORE(xmm7, pDstPixel, store_fct);  \
}

#define COPY16(load_fct, store_fct)  \
{  \
	COPY_LOAD(xmm0, pSrcPixel, load_fct);  \
	COPY_LOAD(xmm1, pSrcPixel, load_fct);  \
	COPY_LOAD(xmm2, pSrcPixel, load_fct);  \
	COPY_LOAD(xmm3, pSrcPixel, load_fct);  \
	COPY_STORE(xmm0, pDstPixel, store_fct);  \
	COPY_STORE(xmm1, pDstPixel, store_fct);  \
	COPY_STORE(xmm2, pDstPixel, store_fct);  \
	COPY_STORE(xmm3, pDstPixel, store_fct);  \
}

#define COPY8(load_fct, store_fct)  \
{  \
	COPY_LOAD(xmm0, pSrcPixel, load_fct);  \
	COPY_LOAD(xmm1, pSrcPixel, load_fct);  \
	COPY_STORE(xmm0, pDstPixel, store_fct);  \
	COPY_STORE(xmm1, pDstPixel, store_fct);  \
}

#define COPY1()  \
{  \
	*(uint32_t *)pDstPixel = *(uint32_t *)pSrcPixel;  \
	pSrcPixel += 4;  \
	pDstPixel += 4;  \
}

#define COPY_REMAINDER()  \
{  \
	switch (x)  \
	{  \
		case 7:  \
			COPY1();  \
		case 6:  \
			COPY1();  \
		case 5:  \
			COPY1();  \
		case 4:  \
			COPY1();  \
		case 3:  \
			COPY1();  \
		case 2:  \
			COPY1();  \
		case 1:  \
			COPY1();  \
	}  \
}

#define DOCOPY(load_fct, store_fct, HANDLE_REMAINDER, nSrcRemainder, nDstRemainder)  \
{  \
	while (nHeight--)  \
	{  \
		x = nWidth;  \
		while (x & ~31)  \
		{  \
			COPY32(load_fct, store_fct);  \
			x -= 32;  \
		}  \
		if (x & ~15)  \
		{  \
			COPY16(load_fct, store_fct);  \
			x -= 16;  \
		}  \
		if (x & ~7)  \
		{  \
			COPY8(load_fct, store_fct);  \
			x -= 8;  \
		}  \
		HANDLE_REMAINDER;  \
		pSrcPixel += nSrcRemainder;  \
		pDstPixel += nDstRemainder;  \
	}  \
}

XppStatus Xpp_Copy_simd(uint8_t* pDstData, int nDstStep, int nXDst, int nYDst,
	int nWidth, int nHeight, uint8_t* pSrcData, int nSrcStep, int nXSrc, int nYSrc)
{
	int x;
	uint8_t* pSrcPixel;
	uint8_t* pDstPixel;
	int nSrcRemainder;
	int nDstRemainder;
	__m128i xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;

	if (nSrcStep < 0)
		nSrcStep = 4 * nWidth;

	if (nDstStep < 0)
		nDstStep = 4 * nWidth;

	nSrcRemainder = nSrcStep - nWidth * 4;
	nDstRemainder = nDstStep - nWidth * 4;

	pSrcPixel = &pSrcData[(nYSrc * nSrcStep) + (nXSrc * 4)];
	pDstPixel = &pDstData[(nYDst * nDstStep) + (nXDst * 4)];

#if BITS == 32 && __APPLE__
	/* For unknown reasons, the memcpy() implementation in OS X is consistently
	   faster than our SSE2 implementation when copying very small blocks using
	   32-bit code. */
	if (nWidth < 16)
	{
		while (nHeight--)
		{
			xpp_memcpy(pDstPixel, pSrcPixel, nWidth * 4);
			pSrcPixel += nSrcStep;
			pDstPixel += nDstStep;
		}
	}
	else
#endif
	if ((((size_t)pSrcPixel | (size_t)pDstPixel) & 0xF) ||
		(nSrcStep | nDstStep) & 0xF || nWidth < 128)
	{
		if (nWidth & 7)
		{
			DOCOPY(_mm_loadu_si128, _mm_storeu_si128, COPY_REMAINDER(),
				nSrcRemainder, nDstRemainder);
		}
		else
		{
			DOCOPY(_mm_loadu_si128, _mm_storeu_si128, , nSrcRemainder,
				nDstRemainder);
		}
	}
	else
	{
		if (nWidth & 7)
		{
			DOCOPY(_mm_load_si128, _mm_stream_si128, COPY_REMAINDER(),
				nSrcRemainder, nDstRemainder);
		}
		else
		{
			DOCOPY(_mm_load_si128, _mm_stream_si128, , nSrcRemainder, nDstRemainder);
		}
		_mm_mfence();
	}

	return XppSuccess;
}

#define MOVE_LOAD(reg, ptr, load_fct)  \
{  \
	ptr -= 16;  \
	reg = load_fct((__m128i *)(ptr));  \
}

#define MOVE_STORE(reg, ptr, store_fct)  \
{  \
	ptr -= 16;  \
	store_fct((__m128i *)(ptr), reg);  \
}

#define MOVE32(load_fct, store_fct)  \
{  \
	MOVE_LOAD(xmm0, pSrcPixel, load_fct);  \
	MOVE_LOAD(xmm1, pSrcPixel, load_fct);  \
	MOVE_LOAD(xmm2, pSrcPixel, load_fct);  \
	MOVE_LOAD(xmm3, pSrcPixel, load_fct);  \
	MOVE_LOAD(xmm4, pSrcPixel, load_fct);  \
	MOVE_LOAD(xmm5, pSrcPixel, load_fct);  \
	MOVE_LOAD(xmm6, pSrcPixel, load_fct);  \
	MOVE_LOAD(xmm7, pSrcPixel, load_fct);  \
	MOVE_STORE(xmm0, pDstPixel, store_fct);  \
	MOVE_STORE(xmm1, pDstPixel, store_fct);  \
	MOVE_STORE(xmm2, pDstPixel, store_fct);  \
	MOVE_STORE(xmm3, pDstPixel, store_fct);  \
	MOVE_STORE(xmm4, pDstPixel, store_fct);  \
	MOVE_STORE(xmm5, pDstPixel, store_fct);  \
	MOVE_STORE(xmm6, pDstPixel, store_fct);  \
	MOVE_STORE(xmm7, pDstPixel, store_fct);  \
}

#define MOVE16(load_fct, store_fct)  \
{  \
	MOVE_LOAD(xmm0, pSrcPixel, load_fct);  \
	MOVE_LOAD(xmm1, pSrcPixel, load_fct);  \
	MOVE_LOAD(xmm2, pSrcPixel, load_fct);  \
	MOVE_LOAD(xmm3, pSrcPixel, load_fct);  \
	MOVE_STORE(xmm0, pDstPixel, store_fct);  \
	MOVE_STORE(xmm1, pDstPixel, store_fct);  \
	MOVE_STORE(xmm2, pDstPixel, store_fct);  \
	MOVE_STORE(xmm3, pDstPixel, store_fct);  \
}

#define MOVE8(load_fct, store_fct)  \
{  \
	MOVE_LOAD(xmm0, pSrcPixel, load_fct);  \
	MOVE_LOAD(xmm1, pSrcPixel, load_fct);  \
	MOVE_STORE(xmm0, pDstPixel, store_fct);  \
	MOVE_STORE(xmm1, pDstPixel, store_fct);  \
}

#define MOVE1()  \
{  \
	pSrcPixel -= 4;  \
	pDstPixel -= 4;  \
	*(uint32_t *)pDstPixel = *(uint32_t *)pSrcPixel;  \
}

#define MOVE_REMAINDER()  \
{  \
	switch (x & 7)  \
	{  \
		case 7:  \
			MOVE1();  \
		case 6:  \
			MOVE1();  \
		case 5:  \
			MOVE1();  \
		case 4:  \
			MOVE1();  \
		case 3:  \
			MOVE1();  \
		case 2:  \
			MOVE1();  \
		case 1:  \
			MOVE1();  \
	}  \
	x = x & ~7;  \
}

#define DOMOVE(load_fct, store_fct, HANDLE_REMAINDER)  \
{  \
	while (nHeight--)  \
	{  \
		x = nWidth;  \
		HANDLE_REMAINDER;  \
		if (x & 8)  \
		{  \
			MOVE8(load_fct, store_fct);  \
			x -= 8;  \
		}  \
		if (x & 16)  \
		{  \
			MOVE16(load_fct, store_fct);  \
			x -= 16;  \
		}  \
		while (x)  \
		{  \
			MOVE32(load_fct, store_fct);  \
			x -= 32;  \
		}  \
		pSrcPixel -= nRemainder;  \
		pDstPixel -= nRemainder;  \
	}  \
}

XppStatus Xpp_Move_simd(uint8_t* pData, int nStep, int nXDst, int nYDst,
	int nWidth, int nHeight, int nXSrc, int nYSrc)
{
	int x;
	uint8_t* pSrcPixel;
	uint8_t* pDstPixel;
	int nRemainder;
	__m128i xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;

	if (nStep < 0)
		nStep = 4 * nWidth;

	nRemainder = nStep - nWidth * 4;

	pSrcPixel = &pData[((nYSrc + nHeight - 1) * nStep) + ((nXSrc + nWidth) * 4)];
	pDstPixel = &pData[((nYDst + nHeight - 1) * nStep) + ((nXDst + nWidth) * 4)];

	if (pSrcPixel > pDstPixel)
	{
		/* NOTE: this is essentially what Xpp_Copy_simd() does, but since
		   that algorithm is designed for copying blocks between different images,
		   it uses non-temporal move instructions to bypass the cache when copying
		   large blocks.  The Move algorithm is intended for moving blocks within
		   the same image, and since those blocks may overlap, we always want to
		   use the cache. */

		pSrcPixel = &pData[(nYSrc * nStep) + (nXSrc * 4)];
		pDstPixel = &pData[(nYDst * nStep) + (nXDst * 4)];

		if (nWidth & 7)
		{
			DOCOPY(_mm_loadu_si128, _mm_storeu_si128, COPY_REMAINDER(),
				nRemainder, nRemainder);
		}
		else
		{
			DOCOPY(_mm_loadu_si128, _mm_storeu_si128, , nRemainder, nRemainder);
		}
	}
	else
	{
		if (nWidth & 7)
		{
			DOMOVE(_mm_loadu_si128, _mm_storeu_si128, MOVE_REMAINDER());
		}
		else
		{
			DOMOVE(_mm_loadu_si128, _mm_storeu_si128, );
		}
	}

	return XppSuccess;
}

#define DOSCALE(pSrcPixel)  \
{  \
	/* xmm0 = (B0 G0 R0 X0 B1 G1 R1 X1 B2 G2 R2 X2 B3 G3 R3 X3) */  \
	xmm0 = _mm_loadu_si128((__m128i *)(pSrcPixel));  \
	/* xmm1 = (B4 G4 R4 X4 B5 G5 R5 X5 B6 G6 R6 X6 B7 G7 R7 X7) */  \
	xmm1 = _mm_loadu_si128((__m128i *)((pSrcPixel) + 16));  \
  \
	/* xmm2 = (B0 G0 R0 X0 B2 G2 R2 X2 B4 G4 R4 X4 B6 G6 R6 X6) = SrcE */  \
	xmm2 = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(xmm0),  \
	                                       _mm_castsi128_ps(xmm1), 0x88));  \
	/* xmm3 = (B1 G1 R1 X1 B3 G3 R3 X3 B5 G5 R5 X5 B7 G7 R7 X7) = SrcO */  \
	xmm3 = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(xmm0),  \
	                                       _mm_castsi128_ps(xmm1), 0xDD));  \
  \
	xmm4 = _mm_setzero_si128();  \
	/* xmm0 = (B0 G0 R0 X0 B2 G2 R2 X2) */  \
	xmm0 = _mm_unpacklo_epi8(xmm2, xmm4);  \
	/* xmm1 = (B1 G1 R1 X1 B3 G3 R3 X3) */  \
	xmm1 = _mm_unpacklo_epi8(xmm3, xmm4);  \
	suml = _mm_add_epi16(suml, xmm0);  \
	suml = _mm_add_epi16(suml, xmm1);  \
  \
	/* xmm0 = (B4 G4 R4 X4 B6 G6 R6 X6) */  \
	xmm0 = _mm_unpackhi_epi8(xmm2, xmm4);  \
	/* xmm1 = (B5 G5 R5 X5 B7 G7 R7 X7) */  \
	xmm1 = _mm_unpackhi_epi8(xmm3, xmm4);  \
	sumh = _mm_add_epi16(sumh, xmm0);  \
	sumh = _mm_add_epi16(sumh, xmm1);  \
}

XppStatus Xpp_CopyFromRetina_simd(uint8_t* pDstData, int nDstStep, int nXDst,
	int nYDst, int nWidth, int nHeight, uint8_t* pSrcData, int nSrcStep, int nXSrc,
	int nYSrc)
{
	int x;
	int nSrcPad;
	int nDstPad;
	uint32_t R, G, B;
	uint8_t* pSrcPixel;
	uint8_t* pDstPixel;
	__m128i xmm0, xmm1, xmm2, xmm3, xmm4, suml, sumh;

	if (nSrcStep < 0)
		nSrcStep = 8 * nWidth;

	if (nDstStep < 0)
		nDstStep = 4 * nWidth;

	nSrcPad = (nSrcStep - (nWidth * 8));
	nDstPad = (nDstStep - (nWidth * 4));

	pSrcPixel = &pSrcData[(nYSrc * nSrcStep) + (nXSrc * 8)];
	pDstPixel = &pDstData[(nYDst * nDstStep) + (nXDst * 4)];

	while (nHeight--)
	{
		x = nWidth;

		while (x & ~3)
		{
			suml = sumh = _mm_setzero_si128();

			DOSCALE(pSrcPixel);
			DOSCALE(pSrcPixel + nSrcStep);

			suml = _mm_srai_epi16(suml, 2);
			sumh = _mm_srai_epi16(sumh, 2);
			xmm0 = _mm_packus_epi16(suml, sumh);
			_mm_storeu_si128((__m128i *)pDstPixel, xmm0);

			pSrcPixel += 32;  pDstPixel += 16;
			x -= 4;
		}

		while (x--)
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

	return XppSuccess;
}
