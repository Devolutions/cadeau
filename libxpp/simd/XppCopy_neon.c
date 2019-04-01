
#include <xpp/copy.h>
#include <xpp/compare.h>

#define XPP_SIMD_INTERNAL
#include "XppSimd.h"
#include <arm_neon.h>

#define COPY_LOAD(reg, ptr)  \
{  \
	reg = vld1q_u32((uint32_t *)(ptr));  \
	ptr += 16;  \
}

#define COPY_STORE(reg, ptr)  \
{  \
	vst1q_u32((uint32_t *)(ptr), reg);  \
	ptr += 16;  \
}

#define COPY32()  \
{  \
	COPY_LOAD(q0, pSrcPixel);  \
	COPY_LOAD(q1, pSrcPixel);  \
	COPY_LOAD(q2, pSrcPixel);  \
	COPY_LOAD(q3, pSrcPixel);  \
	COPY_LOAD(q4, pSrcPixel);  \
	COPY_LOAD(q5, pSrcPixel);  \
	COPY_LOAD(q6, pSrcPixel);  \
	COPY_LOAD(q7, pSrcPixel);  \
	COPY_STORE(q0, pDstPixel);  \
	COPY_STORE(q1, pDstPixel);  \
	COPY_STORE(q2, pDstPixel);  \
	COPY_STORE(q3, pDstPixel);  \
	COPY_STORE(q4, pDstPixel);  \
	COPY_STORE(q5, pDstPixel);  \
	COPY_STORE(q6, pDstPixel);  \
	COPY_STORE(q7, pDstPixel);  \
}

#define COPY16()  \
{  \
	COPY_LOAD(q0, pSrcPixel);  \
	COPY_LOAD(q1, pSrcPixel);  \
	COPY_LOAD(q2, pSrcPixel);  \
	COPY_LOAD(q3, pSrcPixel);  \
	COPY_STORE(q0, pDstPixel);  \
	COPY_STORE(q1, pDstPixel);  \
	COPY_STORE(q2, pDstPixel);  \
	COPY_STORE(q3, pDstPixel);  \
}

#define COPY8()  \
{  \
	COPY_LOAD(q0, pSrcPixel);  \
	COPY_LOAD(q1, pSrcPixel);  \
	COPY_STORE(q0, pDstPixel);  \
	COPY_STORE(q1, pDstPixel);  \
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

#define DOCOPY(HANDLE_REMAINDER, nSrcRemainder, nDstRemainder)  \
{  \
	while (nHeight--)  \
	{  \
		x = nWidth;  \
		while (x & ~31)  \
		{  \
			COPY32();  \
			x -= 32;  \
		}  \
		if (x & ~15)  \
		{  \
			COPY16();  \
			x -= 16;  \
		}  \
		if (x & ~7)  \
		{  \
			COPY8();  \
			x -= 8;  \
		}  \
		HANDLE_REMAINDER;  \
		pSrcPixel += nSrcRemainder;  \
		pDstPixel += nDstRemainder;  \
	}  \
}

#if !defined(__ANDROID__)

XppStatus Xpp_Copy_simd(uint8_t* pDstData, int nDstStep, int nXDst, int nYDst,
	int nWidth, int nHeight, uint8_t* pSrcData, int nSrcStep, int nXSrc, int nYSrc)
{
	int x;
	uint8_t* pSrcPixel;
	uint8_t* pDstPixel;
	int nSrcRemainder;
	int nDstRemainder;
	uint32x4_t q0, q1, q2, q3, q4, q5, q6, q7;

	if (nSrcStep < 0)
		nSrcStep = 4 * nWidth;

	if (nDstStep < 0)
		nDstStep = 4 * nWidth;

	nSrcRemainder = nSrcStep - nWidth * 4;
	nDstRemainder = nDstStep - nWidth * 4;

	pSrcPixel = &pSrcData[(nYSrc * nSrcStep) + (nXSrc * 4)];
	pDstPixel = &pDstData[(nYDst * nDstStep) + (nXDst * 4)];

	if (nWidth & 7)
	{
		DOCOPY(COPY_REMAINDER(), nSrcRemainder, nDstRemainder);
	}
	else
	{
		DOCOPY(, nSrcRemainder, nDstRemainder);
	}

	return XppSuccess;
}

#endif

#define MOVE_LOAD(reg, ptr)  \
{  \
	ptr -= 16;  \
	reg = vld1q_u32((uint32_t *)(ptr));  \
}

#define MOVE_STORE(reg, ptr)  \
{  \
	ptr -= 16;  \
	vst1q_u32((uint32_t *)(ptr), reg);  \
}

#define MOVE32()  \
{  \
	MOVE_LOAD(q0, pSrcPixel);  \
	MOVE_LOAD(q1, pSrcPixel);  \
	MOVE_LOAD(q2, pSrcPixel);  \
	MOVE_LOAD(q3, pSrcPixel);  \
	MOVE_LOAD(q4, pSrcPixel);  \
	MOVE_LOAD(q5, pSrcPixel);  \
	MOVE_LOAD(q6, pSrcPixel);  \
	MOVE_LOAD(q7, pSrcPixel);  \
	MOVE_STORE(q0, pDstPixel);  \
	MOVE_STORE(q1, pDstPixel);  \
	MOVE_STORE(q2, pDstPixel);  \
	MOVE_STORE(q3, pDstPixel);  \
	MOVE_STORE(q4, pDstPixel);  \
	MOVE_STORE(q5, pDstPixel);  \
	MOVE_STORE(q6, pDstPixel);  \
	MOVE_STORE(q7, pDstPixel);  \
}

#define MOVE16()  \
{  \
	MOVE_LOAD(q0, pSrcPixel);  \
	MOVE_LOAD(q1, pSrcPixel);  \
	MOVE_LOAD(q2, pSrcPixel);  \
	MOVE_LOAD(q3, pSrcPixel);  \
	MOVE_STORE(q0, pDstPixel);  \
	MOVE_STORE(q1, pDstPixel);  \
	MOVE_STORE(q2, pDstPixel);  \
	MOVE_STORE(q3, pDstPixel);  \
}

#define MOVE8()  \
{  \
	MOVE_LOAD(q0, pSrcPixel);  \
	MOVE_LOAD(q1, pSrcPixel);  \
	MOVE_STORE(q0, pDstPixel);  \
	MOVE_STORE(q1, pDstPixel);  \
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

#define DOMOVE(HANDLE_REMAINDER)  \
{  \
	while (nHeight--)  \
	{  \
		x = nWidth;  \
		HANDLE_REMAINDER;  \
		if (x & 8)  \
		{  \
			MOVE8();  \
			x -= 8;  \
		}  \
		if (x & 16)  \
		{  \
			MOVE16();  \
			x -= 16;  \
		}  \
		while (x)  \
		{  \
			MOVE32();  \
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
	uint32x4_t q0, q1, q2, q3, q4, q5, q6, q7;

	if (nStep < 0)
		nStep = 4 * nWidth;

	nRemainder = nStep - nWidth * 4;

	pSrcPixel = &pData[((nYSrc + nHeight - 1) * nStep) + ((nXSrc + nWidth) * 4)];
	pDstPixel = &pData[((nYDst + nHeight - 1) * nStep) + ((nXDst + nWidth) * 4)];

	if (pSrcPixel > pDstPixel)
	{
		pSrcPixel = &pData[(nYSrc * nStep) + (nXSrc * 4)];
		pDstPixel = &pData[(nYDst * nStep) + (nXDst * 4)];

		if (nWidth & 7)
		{
			DOCOPY(COPY_REMAINDER(), nRemainder, nRemainder);
		}
		else
		{
			DOCOPY(, nRemainder, nRemainder);
		}
	}
	else
	{
		if (nWidth & 7)
		{
			DOMOVE(MOVE_REMAINDER());
		}
		else
		{
			DOMOVE();
		}
	}

	return XppSuccess;
}

#define DOSCALE(pSrcPixel)  \
{  \
	src0 = vld1q_u32((uint32_t *)(pSrcPixel));  \
	src1 = vld1q_u32((uint32_t *)((pSrcPixel) + 16));  \
	srceo = vuzpq_u32(src0, src1);  \
  \
	srcel = vmovl_u8(vget_low_u8(vreinterpretq_u8_u32(srceo.val[0])));  \
	srcol = vmovl_u8(vget_low_u8(vreinterpretq_u8_u32(srceo.val[1])));  \
	suml = vaddq_u16(suml, srcel);  \
	suml = vaddq_u16(suml, srcol);  \
  \
	srceh = vmovl_u8(vget_high_u8(vreinterpretq_u8_u32(srceo.val[0])));  \
	srcoh = vmovl_u8(vget_high_u8(vreinterpretq_u8_u32(srceo.val[1])));  \
	sumh = vaddq_u16(sumh, srceh);  \
	sumh = vaddq_u16(sumh, srcoh);  \
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
	uint32x4_t src0, src1;
	uint32x4x2_t srceo;
	uint16x8_t srcel, srcol, srceh, srcoh, suml, sumh;

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
			suml = sumh = vdupq_n_u16(0);

			DOSCALE(pSrcPixel);
			DOSCALE(pSrcPixel + nSrcStep);

			suml = vshrq_n_u16(suml, 2);
			sumh = vshrq_n_u16(sumh, 2);
			vst1q_u8(pDstPixel, vcombine_u8(vqmovn_u16(suml), vqmovn_u16(sumh)));

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
