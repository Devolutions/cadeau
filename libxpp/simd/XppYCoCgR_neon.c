
#include <xpp/color.h>

#define XPP_SIMD_INTERNAL
#include "simd.h"
#include <arm_neon.h>

#define DODECODE(pY, pRGB, co, cg, offset)  \
{  \
	y = vld1q_u8(pY);  \
	yl = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(y)));  \
	yh = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(y)));  \
	yeo = vuzpq_s16(yl, yh);  \
  \
	cgs1 = vshrq_n_s16(cg, 1);  \
	tmpe = vsubq_s16(yeo.val[0], cgs1);  \
	tmpo = vsubq_s16(yeo.val[1], cgs1);  \
  \
	ge = vaddq_s16(cg, tmpe);  \
	go = vaddq_s16(cg, tmpo);  \
  \
	cos1 = vshrq_n_s16(co, 1);  \
	be = vsubq_s16(tmpe, cos1);  \
	bo = vsubq_s16(tmpo, cos1);  \
  \
	re = vaddq_s16(be, co);  \
	ro = vaddq_s16(bo, co);  \
  \
	blh = vzipq_s16(be, bo);  \
	pixels.val[0] =  \
		vcombine_u8(vqmovun_s16(blh.val[0]), vqmovun_s16(blh.val[1]));  \
	glh = vzipq_s16(ge, go);  \
	pixels.val[1] =  \
		vcombine_u8(vqmovun_s16(glh.val[0]), vqmovun_s16(glh.val[1]));  \
	rlh = vzipq_s16(re, ro);  \
	pixels.val[2] =  \
		vcombine_u8(vqmovun_s16(rlh.val[0]), vqmovun_s16(rlh.val[1]));  \
	pixels.val[3] = vceqq_u8(pixels.val[2], pixels.val[2]);  \
  \
	vst4q_u8((pRGB), pixels);  \
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
	uint8x16x4_t pixels;
	int16x8x2_t blh, glh, rlh, yeo;
	uint8x16_t y, co, cg;
	int16x8_t yl, yh, cgl, cgh, cgs1, col, coh, cos1, be, bo, ge, go, re, ro;
	int16x8_t tmpe, tmpo, v255;

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

	v255 = vdupq_n_s16(255);

	while (halfHeight--)
	{
		x = fullWidth;

		while (x & ~31)
		{
			co = vld1q_u8(pCo);
			col = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(co)));
			col = vshlq_n_s16(col, 1);
			col = vsubq_s16(col, v255);
			coh = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(co)));
			coh = vshlq_n_s16(coh, 1);
			coh = vsubq_s16(coh, v255);

			cg = vld1q_u8(pCg);
			cgl = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(cg)));
			cgl = vshlq_n_s16(cgl, 1);
			cgl = vsubq_s16(cgl, v255);
			cgh = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(cg)));
			cgh = vshlq_n_s16(cgh, 1);
			cgh = vsubq_s16(cgh, v255);

			DODECODE(pY, pRGB, col, cgl, 0);
			DODECODE(pY + 16, pRGB + 64, coh, cgh, 8);
			DODECODE(pY + srcStep[0], pRGB + dstStep, col, cgl, 0);
			DODECODE(pY + srcStep[0] + 16, pRGB + dstStep + 64, coh, cgh, 8);

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

#if BITS == 32
static __inline int16x8_t __attribute__((__always_inline__))
	vpaddq_s16(int16x8_t a, int16x8_t b)
{
	return vcombine_s16(vpadd_s16(vget_low_s16(a), vget_high_s16(a)),
		vpadd_s16(vget_low_s16(b), vget_high_s16(b)));
}
#endif

#define DOENCODE(pRGB, pY, cosum, cgsum)  \
{  \
	/* pixels[0] = (B0 B1 B2 B3 B4 B5 B6 B7 B8 B9 Ba Bb Bc Bd Be Bf) */  \
	/* pixels[1] = (G0 G1 G2 G3 G4 G5 G6 G7 G8 G9 Ga Gb Gc Gd Ge Gf) */  \
	/* pixels[2] = (R0 R1 R2 R3 R4 R5 R6 R7 R8 R9 Ra Rb Rc Rd Re Rf) */  \
	/* pixels[3] = (X0 X1 X2 X3 X4 X5 X6 X7 X8 X9 Xa Xb Xc Xd Xe Xf) */  \
	pixels = vld4q_u8(pRGB);  \
  \
	bl = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(pixels.val[0])));  \
	bh = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(pixels.val[0])));  \
	gl = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(pixels.val[1])));  \
	gh = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(pixels.val[1])));  \
	rl = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(pixels.val[2])));  \
	rh = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(pixels.val[2])));  \
  \
	col =  vsubq_s16(rl, bl);  \
	coh =  vsubq_s16(rh, bh);  \
	cosum = vaddq_s16(cosum, vpaddq_s16(col, coh));  \
  \
	tmpl = vshrq_n_s16(col, 1);  \
	tmph = vshrq_n_s16(coh, 1);  \
	tmpl = vaddq_s16(tmpl, bl);  \
	tmph = vaddq_s16(tmph, bh);  \
	cgl = vsubq_s16(gl, tmpl);  \
	cgh = vsubq_s16(gh, tmph);  \
	cgsum = vaddq_s16(cgsum, vpaddq_s16(cgl, cgh));  \
  \
	cgl = vshrq_n_s16(cgl, 1);  \
	cgh = vshrq_n_s16(cgh, 1);  \
	yl = vaddq_s16(cgl, tmpl);  \
	yh = vaddq_s16(cgh, tmph);  \
  \
	vst1q_u8(pY, vcombine_u8(vqmovun_s16(yl), vqmovun_s16(yh)));  \
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
	uint8x16x4_t pixels;
	int16x8_t bl, bh, gl, gh, rl, rh, yl, yh, col, coh, cgl, cgh, tmpl, tmph;
	int16x8_t cosum0, cosum1, cgsum0, cgsum1, v1024;

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
			cosum0 = cgsum0 = cosum1 = cgsum1 = vdupq_n_s16(0);
			DOENCODE(pRGB, pY, cosum0, cgsum0);
			DOENCODE(pRGB + 64, pY + 16, cosum1, cgsum1);
			DOENCODE(pRGB + srcStep, pY + dstStep[0], cosum0, cgsum0);
			DOENCODE(pRGB + srcStep + 64, pY + dstStep[0] + 16, cosum1, cgsum1);

			v1024 = vdupq_n_s16(1024);
			cosum0 = vaddq_s16(cosum0, v1024);
			cosum1 = vaddq_s16(cosum1, v1024);
			cosum0 = vshrq_n_s16(cosum0, 3);
			cosum1 = vshrq_n_s16(cosum1, 3);
			vst1q_u8(pCo, vcombine_u8(vqmovun_s16(cosum0), vqmovun_s16(cosum1)));

			cgsum0 = vaddq_s16(cgsum0, v1024);
			cgsum1 = vaddq_s16(cgsum1, v1024);
			cgsum0 = vshrq_n_s16(cgsum0, 3);
			cgsum1 = vshrq_n_s16(cgsum1, 3);
			vst1q_u8(pCg, vcombine_u8(vqmovun_s16(cgsum0), vqmovun_s16(cgsum1)));

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
