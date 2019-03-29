
#include <xpp/copy.h>
#include <xpp/compare.h>

#define XPP_SIMD_INTERNAL
#include "simd.h"
#include "emmintrin.h"

#if !defined(_MSC_VER)
#define _mm_castsi128_ps(a) ((__m128)(a))
#define _mm_castps_si128(a) ((__m128i)(a))
#endif

#ifdef LINEAR_COMPARE32

#define COMPARE32_CMP(index)  \
{  \
	xmm0 = _mm_loadu_si128((__m128i *)(p1 + index));  \
	xmm1 = _mm_loadu_si128((__m128i *)(p2 + index));  \
	xmm2 = _mm_cmpeq_epi8(xmm0, xmm1);  \
	if (_mm_movemask_epi8(xmm2) != 0xFFFF)  \
	{  \
		rowEqual = false;  \
		cols[x / 16] = false;  \
		if (l > x)  \
			l = x;  \
		if (r < x)  \
			r = x;  \
	}  \
}

XppStatus Xpp_Compare32_simd(uint8_t* pData1, int step1, uint8_t* pData2, int step2,
	int width, int height, XppRect* rect)
{
	bool allEqual;
	bool rowEqual;
	int th;
	int x, y, k;
	int width16 = (width & ~0xF);
	int l, t, r, b;
	uint8_t *p1 = pData1, *p2 = pData2;
	bool cols[1024];
	__m128i xmm0, xmm1, xmm2;

	allEqual = true;

	l = width + 1;
	r = -1;
	t = height + 1;
	b = -1;

	if (width != width16)
	{
		for (y = 0; y < height; y += 16)
		{
			rowEqual = true;

			th = min(height - y, 16);

			xpp_memset(cols, 0xFF, sizeof(cols));

			for (k = 0; k < th; k++)
			{
				for (x = 0; x < width16; x += 16)
				{
					if (!cols[x / 16]) continue;

					COMPARE32_CMP(x * 4);
					COMPARE32_CMP(x * 4 + 16);
					COMPARE32_CMP(x * 4 + 32);
					COMPARE32_CMP(x * 4 + 48);
				}

				if (cols[x / 16])
				{
					if (memcmp(&p1[x * 4], &p2[x * 4], (width - x) * 4) != 0)
					{
						rowEqual = false;
						cols[x / 16] = false;

						if (l > x)
							l = x;

						if (r < x)
							r = x;
					}
				}

				p1 += step1;
				p2 += step2;
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

			xpp_memset(cols, 0xFF, sizeof(cols));

			for (k = 0; k < th; k++)
			{
				for (x = 0; x < width; x += 16)
				{
					if (!cols[x / 16]) continue;

					COMPARE32_CMP(x * 4);
					COMPARE32_CMP(x * 4 + 16);
					COMPARE32_CMP(x * 4 + 32);
					COMPARE32_CMP(x * 4 + 48);
				}

				p1 += step1;
				p2 += step2;
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

#define COMPARE32_CMP(index)  \
{  \
	xmm0 = _mm_loadu_si128((__m128i *)(p1 + index));  \
	xmm1 = _mm_loadu_si128((__m128i *)(p2 + index));  \
	xmm2 = _mm_cmpeq_epi8(xmm0, xmm1);  \
	if (_mm_movemask_epi8(xmm2) != 0xFFFF)  \
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
	__m128i xmm0, xmm1, xmm2;

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
					COMPARE32_CMP(0);
					COMPARE32_CMP(16);
					COMPARE32_CMP(32);
					COMPARE32_CMP(48);

					p1 += step1;
					p2 += step2;
				}
			}
			else if (tw >= 12)
			{
				for (k = 0; k < th; k++)
				{
					COMPARE32_CMP(0);
					COMPARE32_CMP(16);
					COMPARE32_CMP(32);

					p1 += step1;
					p2 += step2;
				}
			}
			else if (tw >= 8)
			{
				for (k = 0; k < th; k++)
				{
					COMPARE32_CMP(0);
					COMPARE32_CMP(16);

					p1 += step1;
					p2 += step2;
				}
			}
			else if (tw >= 4)
			{
				for (k = 0; k < th; k++)
				{
					COMPARE32_CMP(0);

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

#define COMPARE8_CMP(mask, offset)  \
{  \
	if ((cmp & mask) != mask)  \
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

XppStatus Xpp_Compare8_simd(uint8_t* pData1, int step1, uint8_t* pData2, int step2,
	int width, int height, XppRect* rect)
{
	int x, y;
	int width16 = (width & ~0xF);
	int l, r, t, b;
	uint8_t* p1 = pData1;
	uint8_t* p2 = pData2;
	uint32_t cmp;
	__m128i xmm0, xmm1, xmm2;

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
				xmm0 = _mm_loadu_si128((__m128i *)&p1[x]);
				xmm1 = _mm_loadu_si128((__m128i *)&p2[x]);
				xmm2 = _mm_cmpeq_epi8(xmm0, xmm1);
				cmp = _mm_movemask_epi8(xmm2);
				COMPARE8_CMP(0xF, 0);
				COMPARE8_CMP(0xF0, 4);
				COMPARE8_CMP(0xF00, 8);
				COMPARE8_CMP(0xF000, 12);
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
				xmm0 = _mm_loadu_si128((__m128i *)&p1[x]);
				xmm1 = _mm_loadu_si128((__m128i *)&p2[x]);
				xmm2 = _mm_cmpeq_epi8(xmm0, xmm1);
				cmp = _mm_movemask_epi8(xmm2);
				COMPARE8_CMP(0xF, 0);
				COMPARE8_CMP(0xF0, 4);
				COMPARE8_CMP(0xF00, 8);
				COMPARE8_CMP(0xF000, 12);
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
