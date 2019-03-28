
#include <xpp/copy.h>
#include <xpp/compare.h>

#define XPP_SIMD_INTERNAL
#include "simd.h"
#include <arm_neon.h>

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

			memset(cols, 0xFF, sizeof(cols));

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

			memset(cols, 0xFF, sizeof(cols));

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


XppStatus Xpp_Compare32_simd(uint8_t* pData1, int step1, uint8_t* pData2, int step2,
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

	return XppSuccess;
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

XppStatus Xpp_Compare8_simd(uint8_t* pData1, int step1, uint8_t* pData2, int step2,
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

	return XppSuccess;
}

#endif
