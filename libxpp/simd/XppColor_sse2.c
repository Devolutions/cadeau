
#include <xpp/color.h>

#define XPP_SIMD_INTERNAL
#include "simd.h"
#include "emmintrin.h"

/* The following functions contain a straightforward port of the libjpeg-turbo
   SSE2 pixel transform code, which is:

   Copyright (C) 1999-2006, MIYASAKA Masaru.

   This software is provided 'as-is', without any express or implied
   warranty.  In no event will the authors be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
      claim that you wrote the original software. If you use this software
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.
   2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original software.
   3. This notice may not be removed or altered from any source distribution.
*/

#define DODECODE(pY, pRGB, co, cg)  \
{  \
	/* NOTE: This code is inspired by the SSE2 color conversion code in  \
	   libjpeg-turbo */  \
  \
	xmm1 = _mm_loadu_si128((__m128i *)(pY));  \
	xmm0 = _mm_cmpeq_epi16(xmm1, xmm1);  \
	xmm0 = _mm_srli_epi16(xmm0, 8);       /* {0xFF 0x00 0xFF 0x00 ..} */  \
	xmm2 = _mm_and_si128(xmm0, xmm1);     /* YE */  \
	xmm3 = _mm_srli_epi16(xmm1, 8);       /* YO */  \
  \
	xmm1 = _mm_srai_epi16(cg, 1);  \
	xmm0 = _mm_sub_epi16(xmm2, xmm1);     /* tmpE */  \
	xmm1 = _mm_sub_epi16(xmm3, xmm1);     /* tmpO */  \
  \
	xmm6 = _mm_add_epi16(cg, xmm0);       /* GE */  \
	xmm7 = _mm_add_epi16(cg, xmm1);       /* GO */  \
  \
	xmm3 = _mm_srai_epi16(co, 1);  \
	xmm2 = _mm_sub_epi16(xmm0, xmm3);     /* BE */  \
	xmm3 = _mm_sub_epi16(xmm1, xmm3);     /* BO */  \
  \
	xmm4 = _mm_add_epi16(xmm2, co);       /* RE */  \
	xmm5 = _mm_add_epi16(xmm3, co);       /* RO */  \
  \
	xmm6 = _mm_packus_epi16(xmm6, xmm6);  /* G(02468ACE********) */  \
	xmm7 = _mm_packus_epi16(xmm7, xmm7);  /* G(13579BDF********) */  \
	xmm2 = _mm_packus_epi16(xmm2, xmm2);  /* B(02468ACE********) */  \
	xmm3 = _mm_packus_epi16(xmm3, xmm3);  /* B(13579BDF********) */  \
	xmm4 = _mm_packus_epi16(xmm4, xmm4);  /* R(02468ACE********) */  \
	xmm5 = _mm_packus_epi16(xmm5, xmm5);  /* R(13579BDF********) */  \
  \
	/* xmm2=(B0 B2 B4 B6 B8 Ba Bc Be **), xmm3=(B1 B3 B5 B7 B9 Bb Bd Bf **) */  \
	/* xmm6=(G0 G2 G4 G6 G8 Ga Gc Ge **), xmm7=(G1 G3 G5 G7 G9 Gb Gd Gf **) */  \
	/* xmm4=(R0 R2 R4 R6 R8 Ra Rc Re **), xmm5=(R1 R3 R5 R7 R9 Rb Rd Rf **) */  \
  \
	xmm0 = _mm_cmpeq_epi8(xmm5, xmm5);    /* {0xFF 0xFF 0xFF 0xFF ..} */  \
	/* xmm2=(B0 G0 B2 G2 B4 G4 B6 G6 B8 G8 Ba Ga Bc Gc Be Ge) */  \
	xmm2 = _mm_unpacklo_epi8(xmm2, xmm6);  \
	/* xmm6=(R0 X0 R2 X2 R4 X4 R6 X6 R8 X8 Ra Xa Rc Xc Re Xe) */  \
	xmm6 = _mm_unpacklo_epi8(xmm4, xmm0);  \
	/* xmm4=(B1 G1 B3 G3 B5 G5 B7 G7 B9 G9 Bb Gb Bd Gd Bf Gf) */  \
	xmm4 = _mm_unpacklo_epi8(xmm3, xmm7);  \
	/* xmm3=(R1 X1 R3 X3 R5 X5 R7 X7 R9 X9 Rb Xb Rd Xd Rf Xf) */  \
	xmm3 = _mm_unpacklo_epi8(xmm5, xmm0);  \
  \
	/* xmm0=(B0 G0 R0 X0 B2 G2 R2 X2 B4 G4 R4 X4 B6 G6 R6 X6) */  \
	xmm0 = _mm_unpacklo_epi16(xmm2, xmm6);  \
	/* xmm1=(B8 G8 R8 X8 Ba Ga Ra Xa Bc Gc Rc Xc Be Ge Re Xe) */  \
	xmm1 = _mm_unpackhi_epi16(xmm2, xmm6);  \
	/* xmm2=(B1 G1 R1 X1 B3 G3 R3 X3 B5 G5 R5 X5 B7 G7 R7 X7) */  \
	xmm2 = _mm_unpacklo_epi16(xmm4, xmm3);  \
	/* xmm3=(B9 G9 R9 X9 Bb Gb Rb Xb Gd Gd Rd Xd Bf Gf Rf Xf) */  \
	xmm3 = _mm_unpackhi_epi16(xmm4, xmm3);  \
  \
	/* xmm4=(B0 G0 R0 X0 B1 G1 R1 X1 B2 G2 R2 X2 B3 G3 R3 X3) */  \
	xmm4 = _mm_unpacklo_epi32(xmm0, xmm2);  \
	/* xmm5=(B4 G4 R4 X4 B5 G5 R5 X5 B6 G6 R6 X6 B7 G7 R7 X7) */  \
	xmm5 = _mm_unpackhi_epi32(xmm0, xmm2);  \
	/* xmm6=(B8 G8 R8 X8 B9 G9 R9 X9 Ba Ga Ra Xa Bb Gb Rb Xb) */  \
	xmm6 = _mm_unpacklo_epi32(xmm1, xmm3);  \
	/* xmm7=(Bc Gc Rc Xc Bd Gd Rd Xd Be Ge Re Xe Bf Gf Rf Xf) */  \
	xmm7 = _mm_unpackhi_epi32(xmm1, xmm3);  \
  \
	_mm_storeu_si128((__m128i *)(pRGB), xmm4);  \
	_mm_storeu_si128((__m128i *)(pRGB + 16), xmm5);  \
	_mm_storeu_si128((__m128i *)(pRGB + 32), xmm6);  \
	_mm_storeu_si128((__m128i *)(pRGB + 48), xmm7);  \
}

#define CLAMP(_val, _min, _max) \
	if (_val < _min) _val = _min; \
	else if (_val > _max) _val = _max;

void Xpp_YCoCgR420ToRGB_8u_P3AC4R_simd(const uint8_t* pSrc[3],
	int srcStep[3], uint8_t* pDst, int dstStep, int width, int height)
{
	uint32_t x;
	uint32_t dstPad;
	uint32_t srcPad[3];
	uint32_t fullWidth;
	uint32_t fullHeight;
	uint32_t halfWidth;
	uint32_t halfHeight;
	const uint8_t* pY;
	const uint8_t* pCo;
	const uint8_t* pCg;
	int R[4];
	int G[4];
	int B[4];
	int Y[4];
	int Co, Cg, t[4];
	uint8_t* pRGB = pDst;
	__m128i xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, col, coh, cgl, cgh;

	pY = pSrc[0];
	pCo = pSrc[1];
	pCg = pSrc[2];

	fullWidth = width & ~0x1;
	fullHeight = height & ~0x1;
	halfWidth = fullWidth >> 1;
	halfHeight = fullHeight >> 1;

	srcPad[0] = (srcStep[0] - fullWidth);
	srcPad[1] = (srcStep[1] - halfWidth);
	srcPad[2] = (srcStep[2] - halfWidth);

	dstPad = (dstStep - (fullWidth * 4));

	while (halfHeight--)
	{
		x = fullWidth;

		while (x & ~31)
		{
			xmm0 = _mm_setzero_si128();
			xmm2 = _mm_set_epi16(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
			xmm1 = _mm_loadu_si128((__m128i *)pCo);
			col = _mm_unpacklo_epi8(xmm1, xmm0);
			col = _mm_slli_epi16(col, 1);
			col = _mm_sub_epi16(col, xmm2);
			coh = _mm_unpackhi_epi8(xmm1, xmm0);
			coh = _mm_slli_epi16(coh, 1);
			coh = _mm_sub_epi16(coh, xmm2);

			xmm1 = _mm_loadu_si128((__m128i *)pCg);
			cgl = _mm_unpacklo_epi8(xmm1, xmm0);
			cgl = _mm_slli_epi16(cgl, 1);
			cgl = _mm_sub_epi16(cgl, xmm2);
			cgh = _mm_unpackhi_epi8(xmm1, xmm0);
			cgh = _mm_slli_epi16(cgh, 1);
			cgh = _mm_sub_epi16(cgh, xmm2);

			DODECODE(pY, pRGB, col, cgl);
			DODECODE(pY + 16, pRGB + 64, coh, cgh);
			DODECODE(pY + srcStep[0], pRGB + dstStep, col, cgl);
			DODECODE(pY + srcStep[0] + 16, pRGB + dstStep + 64, coh, cgh);

			pY += 32;  pCo += 16;  pCg += 16;
			pRGB += 128;
			x -= 32;
		}

		while (x)
		{
			/* 1st pixel */
			Y[0] = pY[0];
			Co = (int) ((*pCo++ << 1) - 0xFF);
			Cg = (int) ((*pCg++ << 1) - 0xFF);

			/* 2nd pixel */
			Y[1] = pY[1];

			pY = (uint8_t*) (((uint8_t*) pY) + srcStep[0]);

			/* 3rd pixel */
			Y[2] = pY[0];

			/* 4th pixel */
			Y[3] = pY[1];

			pY = (uint8_t*) (((uint8_t*) pY) - srcStep[0] + 2);

			t[0] = Y[0] - (Cg >> 1);
			G[0] = Cg + t[0];
			B[0] = t[0] - (Co >> 1);
			R[0] = B[0] + Co;
			CLAMP(R[0], 0, 255);
			CLAMP(G[0], 0, 255);
			CLAMP(B[0], 0, 255);

			t[1] = Y[1] - (Cg >> 1);
			G[1] = Cg + t[1];
			B[1] = t[1] - (Co >> 1);
			R[1] = B[1] + Co;
			CLAMP(R[1], 0, 255);
			CLAMP(G[1], 0, 255);
			CLAMP(B[1], 0, 255);

			t[2] = Y[2] - (Cg >> 1);
			G[2] = Cg + t[2];
			B[2] = t[2] - (Co >> 1);
			R[2] = B[2] + Co;
			CLAMP(R[2], 0, 255);
			CLAMP(G[2], 0, 255);
			CLAMP(B[2], 0, 255);

			t[3] = Y[3] - (Cg >> 1);
			G[3] = Cg + t[3];
			B[3] = t[3] - (Co >> 1);
			R[3] = B[3] + Co;
			CLAMP(R[3], 0, 255);
			CLAMP(G[3], 0, 255);
			CLAMP(B[3], 0, 255);

			/* 1st pixel */
			pRGB[0] = (uint8_t) B[0];
			pRGB[1] = (uint8_t) G[0];
			pRGB[2] = (uint8_t) R[0];
			pRGB[3] = 0xFF;

			/* 2nd pixel */
			pRGB[4] = (uint8_t) B[1];
			pRGB[5] = (uint8_t) G[1];
			pRGB[6] = (uint8_t) R[1];
			pRGB[7] = 0xFF;

			pRGB += dstStep;

			/* 3rd pixel */
			pRGB[0] = (uint8_t) B[2];
			pRGB[1] = (uint8_t) G[2];
			pRGB[2] = (uint8_t) R[2];
			pRGB[3] = 0xFF;

			/* 4th pixel */
			pRGB[4] = (uint8_t) B[3];
			pRGB[5] = (uint8_t) G[3];
			pRGB[6] = (uint8_t) R[3];
			pRGB[7] = 0xFF;

			pRGB = pRGB - dstStep + 8;

			x -= 2;
		}

		pY = (uint8_t*) (((uint8_t*) pY) + srcPad[0] + srcStep[0]);
		pCo = (uint8_t*) (((uint8_t*) pCo) + srcPad[1]);
		pCg = (uint8_t*) (((uint8_t*) pCg) + srcPad[2]);
		pRGB = pRGB + dstPad + dstStep;
	}
}

#define DOENCODE(pRGB, pY, cosum, cgsum)  \
{  \
	/* NOTE: This code is inspired by the SSE2 color conversion code in  \
	   libjpeg-turbo */  \
  \
	/* xmm0 = (B0 G0 R0 X0 B1 G1 R1 X1 B2 G2 R2 X2 B3 G3 R3 X3) */  \
	xmm0 = _mm_loadu_si128((__m128i *)(pRGB));  \
	/* xmm1 = (B4 G4 R4 X4 B5 G5 R5 X5 B6 G6 R6 X6 B7 G7 R7 X7) */  \
	xmm1 = _mm_loadu_si128((__m128i *)((pRGB) + 16));  \
	/* xmm2 = (B8 G8 R8 X8 B9 G9 R9 X9 Ba Ga Ra Xa Bb Gb Rb Xb) */  \
	xmm2 = _mm_loadu_si128((__m128i *)((pRGB) + 32));  \
	/* xmm3 = (Bc Gc Rc Xc Bd Gd Rd Xd Be Ge Re Xe Bf Gf Rf Xf) */  \
	xmm3 = _mm_loadu_si128((__m128i *)((pRGB) + 48));  \
  \
	/* xmm4 = (B0 B4 G0 G4 R0 R4 X0 X4 B1 B5 G1 G5 R1 R5 X1 X5) */  \
	xmm4 = _mm_unpacklo_epi8(xmm0, xmm1);  \
	/* xmm5 = (B2 B6 G2 G6 R2 R6 X2 X6 B3 B7 G3 G7 R3 R7 X3 X7) */  \
	xmm5 = _mm_unpackhi_epi8(xmm0, xmm1);  \
	/* xmm6 = (B8 Bc G8 Gc R8 Rc X8 Xc B9 Bd G9 Gd R9 Rd X9 Xd) */  \
	xmm6 = _mm_unpacklo_epi8(xmm2, xmm3);  \
	/* xmm7 = (Ba Be Ba Be Ga Ge Ga Ge Rb Rf Rb Rf Xb Xf Xb Xf) */  \
	xmm7 = _mm_unpackhi_epi8(xmm2, xmm3);  \
  \
	/* xmm0 = (B0 B4 B8 Bc G0 G4 G8 Gc R0 R4 R8 Rc X0 X4 X8 Xc) */  \
	xmm0 = _mm_unpacklo_epi16(xmm4, xmm6);  \
	/* xmm1 = (B1 B5 B9 Bd G1 G5 G9 Gd R1 R5 R9 Rd X1 X5 X9 Xd) */  \
	xmm1 = _mm_unpackhi_epi16(xmm4, xmm6);  \
	/* xmm2 = (B2 B6 Ba Be G2 G6 Ga Ge R2 R6 Ra Re X2 X6 Xa Xe) */  \
	xmm2 = _mm_unpacklo_epi16(xmm5, xmm7);  \
	/* xmm3 = (B3 B7 Bb Bf G3 G7 Gb Gf R3 R7 Rb Rf X3 X7 Xb Xf) */  \
	xmm3 = _mm_unpackhi_epi16(xmm5, xmm7);  \
  \
	/* xmm4 = (B0 B2 B4 B6 B8 Ba Bc Be G0 G2 G4 G6 G8 Ga Gc Ge) */  \
	xmm4 = _mm_unpacklo_epi8(xmm0, xmm2);  \
	/* xmm5 = (R0 R2 R4 R6 R8 Ra Rc Re X0 X2 X4 X6 X8 Xa Xc Xe) */  \
	xmm5 = _mm_unpackhi_epi8(xmm0, xmm2);  \
	/* xmm6 = (B1 B3 B5 B7 B9 Bb Bd Bf G1 G3 G5 G7 G9 Gb Gd Gf) */  \
	xmm6 = _mm_unpacklo_epi8(xmm1, xmm3);  \
	/* xmm7 = (R1 R3 R5 R7 R9 Rb Rd Rf X1 X3 X5 X7 X9 Xb Xd Xf) */  \
	xmm7 = _mm_unpackhi_epi8(xmm1, xmm3);  \
  \
	xmm0 = _mm_setzero_si128();  \
	/* xmm1 = (R0 R2 R4 R6 R8 Ra Rc Re) = RE */  \
	xmm1 = _mm_unpacklo_epi8(xmm5, xmm0);  \
	/* xmm2 = (R1 R3 R5 R7 R9 Rb Rd Rf) = RO */  \
	xmm2 = _mm_unpacklo_epi8(xmm7, xmm0);  \
	/* xmm3 = (B0 B2 B4 B6 B8 Ba Bc Be) = BE */  \
	xmm3 = _mm_unpacklo_epi8(xmm4, xmm0);  \
	/* xmm4 = (G0 G2 G4 G6 G8 Ga Gc Ge) = GE */  \
	xmm4 = _mm_unpackhi_epi8(xmm4, xmm0);  \
	/* xmm5 = (B1 B3 B5 B7 B9 Bb Bd Bf) = BO */  \
	xmm5 = _mm_unpacklo_epi8(xmm6, xmm0);  \
	/* xmm6 = (G1 G3 G5 G7 G9 Gb Gd Gf) = GO */  \
	xmm6 = _mm_unpackhi_epi8(xmm6, xmm0);  \
  \
	xmm0 = _mm_sub_epi16(xmm1, xmm3);  /* CoE */  \
	xmm1 = _mm_sub_epi16(xmm2, xmm5);  /* CoO */  \
	cosum = _mm_add_epi16(cosum, xmm0);  \
	cosum = _mm_add_epi16(cosum, xmm1);  \
  \
	xmm2 = _mm_srai_epi16(xmm0, 1);  \
	xmm7 = _mm_srai_epi16(xmm1, 1);  \
	xmm2 = _mm_add_epi16(xmm2, xmm3);  /* tmpE */  \
	xmm7 = _mm_add_epi16(xmm7, xmm5);  /* tmpO */  \
	xmm3 = _mm_sub_epi16(xmm4, xmm2);  /* CgE */  \
	xmm5 = _mm_sub_epi16(xmm6, xmm7);  /* CgO */  \
	cgsum = _mm_add_epi16(cgsum, xmm3);  \
	cgsum = _mm_add_epi16(cgsum, xmm5);  \
  \
	xmm4 = _mm_srai_epi16(xmm3, 1);  \
	xmm6 = _mm_srai_epi16(xmm5, 1);  \
	xmm4 = _mm_add_epi16(xmm4, xmm2);  /* YE */  \
	xmm6 = _mm_add_epi16(xmm6, xmm7);  /* YO */  \
  \
	xmm6 = _mm_slli_epi16(xmm6, 8);  \
	xmm4 = _mm_or_si128(xmm4, xmm6);   /* Y */  \
	_mm_storeu_si128((__m128i *)(pY), xmm4);  \
}

void Xpp_RGBToYCoCgR420_8u_P3AC4R_simd(const uint8_t* pSrc, int32_t srcStep,
	uint8_t* pDst[3], int32_t dstStep[3], int width, int height)
{
	uint32_t x;
	uint32_t srcPad;
	uint32_t dstPad[3];
	uint32_t fullWidth;
	uint32_t fullHeight;
	uint32_t halfWidth;
	uint32_t halfHeight;
	uint8_t* pY;
	uint8_t* pCo;
	uint8_t* pCg;
	int R[4];
	int G[4];
	int B[4];
	int Y[4];
	int Co[4];
	int Cg[4];
	int t[4];
	int sCo, sCg;
	const uint8_t* pRGB = pSrc;
	__m128i xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;
	__m128i cosum0, cosum1, cgsum0, cgsum1;

	pY = pDst[0];
	pCo = pDst[1];
	pCg = pDst[2];

	fullWidth = width & ~0x1;
	fullHeight = height & ~0x1;
	halfWidth = fullWidth >> 1;
	halfHeight = fullHeight >> 1;

	srcPad = (srcStep - (fullWidth * 4));

	dstPad[0] = (dstStep[0] - fullWidth);
	dstPad[1] = (dstStep[1] - halfWidth);
	dstPad[2] = (dstStep[2] - halfWidth);

	while (halfHeight--)
	{
		x = fullWidth;

		while (x & ~31)
		{
			cosum0 = cgsum0 = cosum1 = cgsum1 = _mm_setzero_si128();
			DOENCODE(pRGB, pY, cosum0, cgsum0);
			DOENCODE(pRGB + 64, pY + 16, cosum1, cgsum1);
			DOENCODE(pRGB + srcStep, pY + dstStep[0], cosum0, cgsum0);
			DOENCODE(pRGB + srcStep + 64, pY + dstStep[0] + 16, cosum1, cgsum1);

			xmm0 = _mm_set_epi16(1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024);
			cosum0 = _mm_add_epi16(cosum0, xmm0);
			cosum1 = _mm_add_epi16(cosum1, xmm0);
			cosum0 = _mm_srai_epi16(cosum0, 3);
			cosum1 = _mm_srai_epi16(cosum1, 3);
			xmm1 = _mm_packus_epi16(cosum0, cosum1);
			_mm_storeu_si128((__m128i *)pCo, xmm1);

			cgsum0 = _mm_add_epi16(cgsum0, xmm0);
			cgsum1 = _mm_add_epi16(cgsum1, xmm0);
			cgsum0 = _mm_srai_epi16(cgsum0, 3);
			cgsum1 = _mm_srai_epi16(cgsum1, 3);
			xmm1 = _mm_packus_epi16(cgsum0, cgsum1);
			_mm_storeu_si128((__m128i *)pCg, xmm1);

			pY += 32;  pCo += 16;  pCg += 16;
			pRGB += 128;
			x -= 32;
		}

		while (x)
		{
			/* 1st pixel */
			B[0] = pRGB[0];
			G[0] = pRGB[1];
			R[0] = pRGB[2];

			/* 2nd pixel */
			B[1] = pRGB[4];
			G[1] = pRGB[5];
			R[1] = pRGB[6];

			pRGB += srcStep;

			/* 3rd pixel */
			B[2] = pRGB[0];
			G[2] = pRGB[1];
			R[2] = pRGB[2];

			/* 4th pixel */
			B[3] = pRGB[4];
			G[3] = pRGB[5];
			R[3] = pRGB[6];

			pRGB = pRGB - srcStep + 8;

			Co[0] = R[0] - B[0];
			t[0] = B[0] + (Co[0] >> 1);
			Cg[0] = G[0] - t[0];
			Y[0] = t[0] + (Cg[0] >> 1);

			Co[1] = R[1] - B[1];
			t[1] = B[1] + (Co[1] >> 1);
			Cg[1] = G[1] - t[1];
			Y[1] = t[1] + (Cg[1] >> 1);

			Co[2] = R[2] - B[2];
			t[2] = B[2] + (Co[2] >> 1);
			Cg[2] = G[2] - t[2];
			Y[2] = t[2] + (Cg[2] >> 1);

			Co[3] = R[3] - B[3];
			t[3] = B[3] + (Co[3] >> 1);
			Cg[3] = G[3] - t[3];
			Y[3] = t[3] + (Cg[3] >> 1);

			sCo = (Co[0] + Co[1] + Co[2] + Co[3] + 1024) >> 3;
			sCg = (Cg[0] + Cg[1] + Cg[2] + Cg[3] + 1024) >> 3;

			/* 1st pixel */
			pY[0] = (uint8_t) Y[0];
			*pCo++ = (uint8_t) sCo;
			*pCg++ = (uint8_t) sCg;

			/* 2nd pixel */
			pY[1] = (uint8_t) Y[1];

			pY = pY + dstStep[0];

			/* 3rd pixel */
			pY[0] = (uint8_t) Y[2];

			/* 4th pixel */
			pY[1] = (uint8_t) Y[3];

			pY = pY - dstStep[0] + 2;

			x-=2;
		}

		pRGB = pRGB + srcPad + srcStep;
		pY = pY + dstPad[0] + dstStep[0];
		pCo = pCo + dstPad[1];
		pCg = pCg + dstPad[2];
	}
}
