#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <xpp/math.h>
#include <xpp/color.h>
#include <xpp/primitive.h>

#include <xpp/compare.h>

XppStatus Xpp_Compare32_generic(uint8_t* pData1, int32_t step1, uint8_t* pData2,
    int32_t step2, int32_t width, int32_t height, XppRect* rect)
{
    bool equal;
    bool allEqual;
    int32_t tw, th;
    int32_t tx, ty, k;
    int32_t nrow, ncol;
    int32_t l, t, r, b;
    int32_t left, top;
    int32_t right, bottom;
    uint8_t *p1, *p2;
    bool rows[1024];

    allEqual = true;
    xpp_memset(rows, 0xFF, sizeof(rows));

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

XppStatus Xpp_Compare32(uint8_t* pData1, int32_t step1, uint8_t* pData2, int32_t step2,
        int32_t width, int32_t height, XppRect* rect)
{
    XppPrimitives* primitives = XppPrimitives_Get();
    return primitives->Compare32(pData1, step1, pData2, step2, width, height, rect);
}

XppStatus Xpp_Compare8_generic(uint8_t* pData1, int32_t step1, uint8_t* pData2, int32_t step2,
        int32_t width, int32_t height, XppRect* rect)
{
    int32_t x, y;
    int32_t width4 = (width & ~0x3);
    int32_t l, r, t, b;
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

XppStatus Xpp_Compare8(uint8_t* pData1, int32_t step1, uint8_t* pData2, int32_t step2,
        int32_t width, int32_t height, XppRect* rect)
{
    XppPrimitives* primitives = XppPrimitives_Get();
    return primitives->Compare8(pData1, step1, pData2, step2, width, height, rect);
}
