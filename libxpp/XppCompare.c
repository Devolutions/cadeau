#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <xpp/math.h>
#include <xpp/color.h>
#include <xpp/primitive.h>

#include <xpp/compare.h>

#ifdef WITH_SIMD
#include "simd/simd.h"
#endif

#ifdef WITH_HALIDE
#include <halide/Copy.h>
#include <halide/Compare32Stage1.h>
#include <halide/Compare8Stage1.h>
#include <HalideRuntime.h>
#endif

#ifdef WITH_HALIDE

static void codec_setup_u32_buffer_t(buffer_t* buffer, uint32_t* data, int width, int height, int stride)
{
	memset(buffer, 0, sizeof(buffer_t));

	buffer->extent[0] = width;
	buffer->extent[1] = height;
	buffer->elem_size = 4;
	buffer->host = (uint8_t*)data;
	buffer->stride[0] = 1;
	buffer->stride[1] = stride;
	buffer->host_dirty = true;
}

static void codec_setup_1d_u32_buffer_t(buffer_t* buffer, uint32_t* data, int width)
{
	memset(buffer, 0, sizeof(buffer_t));

	buffer->extent[0] = width;
	buffer->elem_size = 4;
	buffer->host = (uint8_t*)data;
	buffer->stride[0] = 1;
	buffer->host_dirty = true;
}

static void codec_setup_u8_buffer_t(buffer_t* buffer, uint8_t* data, int width, int height, int stride)
{
	memset(buffer, 0, sizeof(buffer_t));

	buffer->extent[0] = width;
	buffer->extent[1] = height;
	buffer->elem_size = 4;
	buffer->host = data;
	buffer->stride[0] = 1;
	buffer->stride[1] = stride;
	buffer->host_dirty = true;
}

static void codec_setup_1d_u8_buffer_t(buffer_t* buffer, uint8_t* data, int width)
{
	memset(buffer, 0, sizeof(buffer_t));

	buffer->extent[0] = width;
	buffer->elem_size = 1;
	buffer->host = data;
	buffer->stride[0] = 1;
	buffer->host_dirty = true;
}

int Xpp_Compare32_halide(uint8_t* pData1, int step1, uint8_t* pData2, int step2, int width, int height,
			      XppRect* rect)
{
	int i;

	buffer_t frame1Buffer;
	buffer_t frame2Buffer;
	buffer_t diffXBuffer;
	buffer_t diffYBuffer;

	uint32_t* diffX = calloc(width, sizeof(uint32_t));
	uint32_t* diffY = calloc(height, sizeof(uint32_t));

	codec_setup_u32_buffer_t(&frame1Buffer, (uint32_t*)pData1, width, height, step1 / sizeof(uint32_t));
	codec_setup_u32_buffer_t(&frame2Buffer, (uint32_t*)pData2, width, height, step2 / sizeof(uint32_t));
	codec_setup_1d_u32_buffer_t(&diffXBuffer, diffX, width);
	codec_setup_1d_u32_buffer_t(&diffYBuffer, diffY, height);

	Compare32Stage1_old_buffer_t(width, height, &frame1Buffer, &frame2Buffer, &diffXBuffer, &diffYBuffer);

	rect->left = width - 1;
	rect->bottom = height - 1;

	for (i = 0; i < width; i++)
	{
		if (diffX[0] == 0) rect->left = min(rect->left, i);
		if (diffX[0] != 0) rect->right = max(rect->right, i);
	}

	for (i = 0; i < width; i++)
	{
		if (diffY[0] == 0) rect->top = min(rect->top, i);
		if (diffY[0] != 0) rect->bottom = max(rect->bottom, i);
	}

	free(diffX);
	free(diffY);

	return 0;
}

int Xpp_Compare8_halide(uint8_t* pData1, int step1, uint8_t* pData2, int step2, int width, int height,
			     XppRect* rect)
{
	int i;

	buffer_t frame1Buffer;
	buffer_t frame2Buffer;
	buffer_t diffXBuffer;
	buffer_t diffYBuffer;

	uint8_t* diffX = calloc(width, 1);
	uint8_t* diffY = calloc(height, 1);

	codec_setup_u8_buffer_t(&frame1Buffer, pData1, width, height, step1);
	codec_setup_u8_buffer_t(&frame2Buffer, pData2, width, height, step2);
	codec_setup_1d_u8_buffer_t(&diffXBuffer, diffX, width);
	codec_setup_1d_u8_buffer_t(&diffYBuffer, diffY, height);

	Compare8Stage1_old_buffer_t(width, height, &frame1Buffer, &frame2Buffer, &diffXBuffer, &diffYBuffer);

	rect->left = width - 1;
	rect->bottom = height - 1;

	for (i = 0; i < width; i++)
	{
		if (diffX[0] == 0) rect->left = min(rect->left, i);
		if (diffX[0] != 0) rect->right = max(rect->right, i);
	}

	for (i = 0; i < width; i++)
	{
		if (diffY[0] == 0) rect->top = min(rect->top, i);
		if (diffY[0] != 0) rect->bottom = max(rect->bottom, i);
	}

	free(diffX);
	free(diffY);

	return 0;
}

#endif

int Xpp_Compare32_c(uint8_t* pData1, int step1, uint8_t* pData2,
	int step2, int width, int height, XppRect* rect)
{
	bool equal;
	bool allEqual;
	int tw, th;
	int tx, ty, k;
	int nrow, ncol;
	int l, t, r, b;
	int left, top;
	int right, bottom;
	uint8_t *p1, *p2;
	bool rows[1024];

	allEqual = true;
	memset(rows, 0xFF, sizeof(rows));

	nrow = (height + 15) / 16;
	ncol = (width + 15) / 16;

	l = ncol + 1;
	r = -1;

	t = nrow + 1;
	b = -1;

	for (ty = 0; ty < nrow; ty++)
	{
		th = ((ty + 1) == nrow) ? (height % 16) : 16;

		if (!th)
			th = 16;

		for (tx = 0; tx < ncol; tx++)
		{
			equal = true;

			tw = ((tx + 1) == ncol) ? (width % 16) : 16;

			if (!tw)
				tw = 16;

			p1 = &pData1[(ty * 16 * step1) + (tx * 16 * 4)];
			p2 = &pData2[(ty * 16 * step2) + (tx * 16 * 4)];

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

			if (!equal)
			{
				rows[ty] = false;

				if (l > tx)
					l = tx;

				if (r < tx)
					r = tx;
			}
		}

		if (!rows[ty])
		{
			allEqual = false;

			if (t > ty)
				t = ty;

			if (b < ty)
				b = ty;
		}
	}

	if (allEqual)
	{
		rect->left = rect->top = 0;
		rect->right = rect->bottom = 0;
		return 0;
	}

	left = l * 16;
	top = t * 16;
	right = (r + 1) * 16;
	bottom = (b + 1) * 16;

	if (right > width)
		right = width;

	if (bottom > height)
		bottom = height;

	rect->left = left;
	rect->top = top;
	rect->right = right;
	rect->bottom = bottom;

	return 1;
}

int Xpp_Compare32(uint8_t* pData1, int step1, uint8_t* pData2, int step2, int width, int height,
		       XppRect* rect)
{
	XppPrimitives* primitives = XppPrimitives_Get();
	return primitives->Compare32(pData1, step1, pData2, step2, width, height, rect);
}

int Xpp_Compare8_c(uint8_t* pData1, int step1, uint8_t* pData2, int step2, int width, int height,
			XppRect* rect)
{
	int x, y;
	int width4 = (width & ~0x3);
	int l, r, t, b;
	uint8_t* p1 = pData1;
	uint8_t* p2 = pData2;

	l = width + 1;
	r = -1;
	t = height + 1;
	b = -1;

	if (width4 != width)
	{
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width4; x += 4)
			{
				if (*(uint32_t *)&p1[x] != *(uint32_t *)&p2[x])
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
			for (x = width4; x < width; x++)
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
			for (x = 0; x < width; x += 4)
			{
				if (*(uint32_t *)&p1[x] != *(uint32_t *)&p2[x])
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

int Xpp_Compare8(uint8_t* pData1, int step1, uint8_t* pData2, int step2, int width, int height,
		      XppRect* rect)
{
	XppPrimitives* primitives = XppPrimitives_Get();
	return primitives->Compare8(pData1, step1, pData2, step2, width, height, rect);
}

int Xpp_Compare(uint8_t* pData1, int nStep1, int nWidth, int nHeight, uint8_t* pData2, int nStep2,
		     XppRect* rect)
{
	return Xpp_Compare32(pData1, nStep1, pData2, nStep2, nWidth, nHeight, rect);
}
