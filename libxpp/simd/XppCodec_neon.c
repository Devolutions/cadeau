
#include <xpp/codec.h>

#define XPP_SIMD_INTERNAL
#include "simd.h"
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

int Xpp_Copy_simd(uint8_t* pDstData, int nDstStep, int nXDst, int nYDst,
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

	return 1;
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

int Xpp_Move_simd(uint8_t* pData, int nStep, int nXDst, int nYDst,
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

	return 1;
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

int Xpp_CopyFromRetina_simd(uint8_t* pDstData, int nDstStep, int nXDst,
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

	return 1;
}

#ifdef LINEAR_COMPARE32

#define COMPARE32_CMP(index, index2, index3, index4)  \
{  \
	q0 = vld1q_u32((uint32_t *)(p1 + index));  \
	q1 = vld1q_u32((uint32_t *)(p1 + index2));  \
	q2 = vld1q_u32((uint32_t *)(p1 + index3));  \
	q3 = vld1q_u32((uint32_t *)(p1 + index4));  \
	q4 = vld1q_u32((uint32_t *)(p2 + index));  \
	q5 = vld1q_u32((uint32_t *)(p2 + index2));  \
	q6 = vld1q_u32((uint32_t *)(p2 + index3));  \
	q7 = vld1q_u32((uint32_t *)(p2 + index4));  \
	q8 = vceqq_u32(q0, q4);  \
	q0 = vceqq_u32(q1, q5);  \
	q1 = vceqq_u32(q2, q6);  \
	q2 = vceqq_u32(q3, q7);  \
	q8 = vandq_u32(q8, q0);  \
	q8 = vandq_u32(q8, q1);  \
	q8 = vandq_u32(q8, q2);  \
	d0 = vand_u32(vget_low_u32(q8), vget_high_u32(q8));  \
	d0 = vpmin_u32(d0, d0);  \
	if (vget_lane_u32(d0, 0) != 0xFFFFFFFF)  \
	{  \
		rowEqual = false;  \
		cols[x / 16] = false;  \
		if (l > x)  \
			l = x;  \
		if (r < x)  \
			r = x;  \
	}  \
}

int Xpp_Compare32_simd(uint8_t* pData1, int step1, uint8_t* pData2, int step2,
	int width, int height, XppRect* rect)
{
	bool allEqual;
	bool rowEqual;
	int th;
	int x, y, k;
	int width16 = (width & ~0xF);
	int remainder1 = step1 - width * 4;
	int remainder2 = step2 - width * 4;
	int l, t, r, b;
	uint8_t *p1 = pData1, *p2 = pData2;
	bool cols[1024];
	uint32x4_t q0, q1, q2, q3, q4, q5, q6, q7, q8;
	uint32x2_t d0;

	allEqual = true;

	l = width + 1;
	r = -1;
	t = height + 1;
	b = -1;

	if (width != width16)
	{
		remainder1 += (width - width16) * 4;
		remainder2 += (width - width16) * 4;

		for (y = 0; y < height; y += 16)
		{
			rowEqual = true;

			th = min(height - y, 16);

			FillMemory(cols, sizeof(cols), 0xFF);

			for (k = 0; k < th; k++)
			{
				for (x = 0; x < width16; x += 16, p1 += 64, p2 += 64)
				{
					if (!cols[x / 16]) continue;

					COMPARE32_CMP(0, 16, 32, 48);
				}

				if (cols[x / 16])
				{
					if (memcmp(p1, p2, (width - x) * 4) != 0)
					{
						rowEqual = false;
						cols[x / 16] = false;

						if (l > x)
							l = x;

						if (r < x)
							r = x;
					}
				}

				p1 += remainder1;
				p2 += remainder2;
			}

			if (!rowEqual)
			{
				allEqual = false;

				if (t > y)
					t = y;

				if (b < y)
					b = y;
			}
		}
	}
	else
	{
		for (y = 0; y < height; y += 16)
		{
			rowEqual = true;

			th = min(height - y, 16);

			FillMemory(cols, sizeof(cols), 0xFF);

			for (k = 0; k < th; k++)
			{
				for (x = 0; x < width16; x += 16, p1 += 64, p2 += 64)
				{
					if (!cols[x / 16]) continue;

					COMPARE32_CMP(0, 16, 32, 48);
				}

				p1 += remainder1;
				p2 += remainder2;
			}

			if (!rowEqual)
			{
				allEqual = false;

				if (t > y)
					t = y;

				if (b < y)
					b = y;
			}
		}
	}

	if (allEqual)
	{
		rect->left = rect->top = 0;
		rect->right = rect->bottom = 0;
		return 0;
	}

	r++;
	b++;

	l &= ~0xF;
	t &= ~0xF;
	r = (r + 15) & ~0xF;
	b = (b + 15) & ~0xF;

	if (r > width)
		r = width;

	if (b > height)
		b = height;

	rect->left = l;
	rect->top = t;
	rect->right = r;
	rect->bottom = b;

	return 1;
}

#else


#define COMPARE32_CMP4(index, index2, index3, index4)  \
{  \
	q0 = vld1q_u32((uint32_t *)(p1 + index));  \
	q1 = vld1q_u32((uint32_t *)(p1 + index2));  \
	q2 = vld1q_u32((uint32_t *)(p1 + index3));  \
	q3 = vld1q_u32((uint32_t *)(p1 + index4));  \
	q4 = vld1q_u32((uint32_t *)(p2 + index));  \
	q5 = vld1q_u32((uint32_t *)(p2 + index2));  \
	q6 = vld1q_u32((uint32_t *)(p2 + index3));  \
	q7 = vld1q_u32((uint32_t *)(p2 + index4));  \
	q8 = vceqq_u32(q0, q4);  \
	q0 = vceqq_u32(q1, q5);  \
	q1 = vceqq_u32(q2, q6);  \
	q2 = vceqq_u32(q3, q7);  \
	q8 = vandq_u32(q8, q0);  \
	q8 = vandq_u32(q8, q1);  \
	q8 = vandq_u32(q8, q2);  \
	d0 = vand_u32(vget_low_u32(q8), vget_high_u32(q8));  \
	d0 = vpmin_u32(d0, d0);  \
	if (vget_lane_u32(d0, 0) != 0xFFFFFFFF)  \
	{  \
		equal = false;  \
		break;  \
	}  \
}

#define COMPARE32_CMP3(index, index2, index3)  \
{  \
	q0 = vld1q_u32((uint32_t *)(p1 + index));  \
	q1 = vld1q_u32((uint32_t *)(p1 + index2));  \
	q2 = vld1q_u32((uint32_t *)(p1 + index3));  \
	q3 = vld1q_u32((uint32_t *)(p2 + index));  \
	q4 = vld1q_u32((uint32_t *)(p2 + index2));  \
	q5 = vld1q_u32((uint32_t *)(p2 + index3));  \
	q6 = vceqq_u32(q0, q3);  \
	q0 = vceqq_u32(q1, q4);  \
	q1 = vceqq_u32(q2, q5);  \
	q6 = vandq_u32(q6, q0);  \
	q6 = vandq_u32(q6, q1);  \
	d0 = vand_u32(vget_low_u32(q6), vget_high_u32(q6));  \
	d0 = vpmin_u32(d0, d0);  \
	if (vget_lane_u32(d0, 0) != 0xFFFFFFFF)  \
	{  \
		equal = false;  \
		break;  \
	}  \
}

#define COMPARE32_CMP2(index, index2)  \
{  \
	q0 = vld1q_u32((uint32_t *)(p1 + index));  \
	q1 = vld1q_u32((uint32_t *)(p1 + index2));  \
	q2 = vld1q_u32((uint32_t *)(p2 + index));  \
	q3 = vld1q_u32((uint32_t *)(p2 + index2));  \
	q4 = vceqq_u32(q0, q2);  \
	q0 = vceqq_u32(q1, q3);  \
	q4 = vandq_u32(q4, q0);  \
	d0 = vand_u32(vget_low_u32(q4), vget_high_u32(q4));  \
	d0 = vpmin_u32(d0, d0);  \
	if (vget_lane_u32(d0, 0) != 0xFFFFFFFF)  \
	{  \
		equal = false;  \
		break;  \
	}  \
}


int Xpp_Compare32_simd(uint8_t* pData1, int step1, uint8_t* pData2, int step2,
	int width, int height, XppRect* rect)
{
	bool equal;
	bool allEqual;
	bool rowEqual;
	int tw, th;
	int x, y, k;
	int l, t, r, b;
	uint8_t *p1, *p2;
	uint32x4_t q0, q1, q2, q3, q4, q5, q6, q7, q8;
	uint32x2_t d0;

	allEqual = true;

	l = width + 1;
	r = -1;
	t = height + 1;
	b = -1;

	for (y = 0; y < height; y += 16)
	{
		rowEqual = true;

		th = min(height - y, 16);

		for (x = 0; x < width; x += 16)
		{
			equal = true;

			tw = min(width - x, 16);

			p1 = &pData1[(y * step1) + (x * 4)];
			p2 = &pData2[(y * step2) + (x * 4)];

			if (tw >= 16)
			{
				for (k = 0; k < th; k++)
				{
					COMPARE32_CMP4(0, 16, 32, 48);

					p1 += step1;
					p2 += step2;
				}
			}
			else if (tw >= 12)
			{
				for (k = 0; k < th; k++)
				{
					COMPARE32_CMP3(0, 16, 32);

					p1 += step1;
					p2 += step2;
				}
			}
			else if (tw >= 8)
			{
				for (k = 0; k < th; k++)
				{
					COMPARE32_CMP2(0, 16);

					p1 += step1;
					p2 += step2;
				}
			}
			else
			{
				for (k = 0; k < th; k++)
				{
					if (memcmp(p1, p2, tw * 4) != 0)
					{
						equal = false;
						break;
					}

					p1 += step1;
					p2 += step2;
				}
			}

			if (!equal)
			{
				rowEqual = false;

				if (l > x)
					l = x;

				if (r < x)
					r = x;
			}
		}

		if (!rowEqual)
		{
			allEqual = false;

			if (t > y)
				t = y;

			if (b < y)
				b = y;
		}
	}

	if (allEqual)
	{
		rect->left = rect->top = 0;
		rect->right = rect->bottom = 0;
		return 0;
	}

	r++;
	b++;

	l &= ~0xF;
	t &= ~0xF;
	r = (r + 15) & ~0xF;
	b = (b + 15) & ~0xF;

	if (r > width)
		r = width;

	if (b > height)
		b = height;

	rect->left = l;
	rect->top = t;
	rect->right = r;
	rect->bottom = b;

	return 1;
}

#endif

#define COMPARE8_CMP(index, offset)  \
{  \
	if (cmp[index] != 0xFFFFFFFF)  \
	{  \
		if (x + offset < l)  \
			l = x + offset;  \
  \
		if (x + offset > r)  \
			r = x + offset;  \
  \
		if (y < t)  \
			t = y;  \
  \
		if (y > b)  \
			b = y;  \
	}  \
}

#if !defined(__APPLE__) || !defined(__aarch64__)

int Xpp_Compare8_simd(uint8_t* pData1, int step1, uint8_t* pData2, int step2,
	int width, int height, XppRect* rect)
{
	int x, y;
	int width16 = (width & ~0xF);
	int l, r, t, b;
	uint8_t* p1 = pData1;
	uint8_t* p2 = pData2;
	uint32_t cmp[4];
	uint32x4_t q0, q1, q2;

	l = width + 1;
	r = -1;
	t = height + 1;
	b = -1;

	if (width16 != width)
	{
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width16; x += 16)
			{
				q0 = vld1q_u32((uint32_t *)&p1[x]);
				q1 = vld1q_u32((uint32_t *)&p2[x]);
				q2 = vceqq_u32(q0, q1);
				vst1q_u32((uint32_t *)cmp, q2);
				COMPARE8_CMP(0, 0);
				COMPARE8_CMP(1, 4);
				COMPARE8_CMP(2, 8);
				COMPARE8_CMP(3, 12);
			}
			for (x = width16; x < width; x++)
			{
				if (p1[x] != p2[x])
				{
					if (x < l)
						l = x;

					if (x > r)
						r = x;

					if (y < t)
						t = y;

					if (y > b)
						b = y;
				}
			}

			p1 += step1;
			p2 += step2;
		}
	}
	else
	{
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width16; x += 16)
			{
				q0 = vld1q_u32((uint32_t *)&p1[x]);
				q1 = vld1q_u32((uint32_t *)&p2[x]);
				q2 = vceqq_u32(q0, q1);
				vst1q_u32((uint32_t *)cmp, q2);
				COMPARE8_CMP(0, 0);
				COMPARE8_CMP(1, 4);
				COMPARE8_CMP(2, 8);
				COMPARE8_CMP(3, 12);
			}

			p1 += step1;
			p2 += step2;
		}
	}

	if ((r == -1) && (b == -1))
	{
		rect->left = 0;
		rect->top = 0;
		rect->right = 0;
		rect->bottom = 0;
		return 0;
	}

	r++;
	b++;

	l &= ~0x3;
	t &= ~0x3;
	r = (r + 3) & ~0x3;
	b = (b + 3) & ~0x3;

	rect->left = l;
	rect->top = t;
	rect->right = r;
	rect->bottom = b;

	return 1;
}

#endif
