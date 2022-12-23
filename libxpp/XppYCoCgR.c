#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <xpp/math.h>
#include <xpp/primitive.h>

#include <xpp/color.h>

/* 16-bit signed YCoCg-R */

XppStatus Xpp_RGBToYCoCgR_16s_P3AC4R_generic(const uint8_t* pSrc, uint32_t srcStep,
    int16_t* pDst[3], uint32_t dstStep[3], uint32_t width, uint32_t height)
{
    int32_t x, y;
    int32_t srcPad;
    int32_t dstPad[3];
    int16_t* pY;
    int16_t* pCo;
    int16_t* pCg;
    int32_t R, G, B;
    int32_t Y, Co, Cg, t;
    const uint8_t* pRGB = pSrc;

    pY = pDst[0];
    pCo = pDst[1];
    pCg = pDst[2];

    srcPad = (srcStep - (width * 4));
    dstPad[0] = (dstStep[0] - (width * 2));
    dstPad[1] = (dstStep[1] - (width * 2));
    dstPad[2] = (dstStep[2] - (width * 2));

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            B = pRGB[0];
            G = pRGB[1];
            R = pRGB[2];
            pRGB += 4;

            Co = R - B;
            t = B + (Co >> 1);
            Cg = G - t;
            Y = t + (Cg >> 1);

            Y -= 128;

            /**
             * Y:  [-128, 127]
             * Co: [-256, 255]
             * Cg: [-256, 255]
             */

            *pY = (int16_t) Y;
            *pCo = (int16_t) Co;
            *pCg = (int16_t) Cg;

            pY++;
            pCo++;
            pCg++;
        }

        pRGB += srcPad;
        pY = (int16_t*) (((uint8_t*) pY) + dstPad[0]);
        pCo = (int16_t*) (((uint8_t*) pCo) + dstPad[1]);
        pCg = (int16_t*) (((uint8_t*) pCg) + dstPad[2]);
    }

    return XppSuccess;
}

XppStatus Xpp_YCoCgRToRGB_16s_P3AC4R_generic(const int16_t* pSrc[3], uint32_t srcStep[3],
                uint8_t* pDst, uint32_t dstStep, uint32_t width, uint32_t height)
{
    int32_t x, y;
    int32_t dstPad;
    int32_t srcPad[3];
    const int16_t* pY;
    const int16_t* pCo;
    const int16_t* pCg;
    int32_t R, G, B;
    int32_t Y, Co, Cg, t;
    uint8_t* pRGB = pDst;

    pY = pSrc[0];
    pCo = pSrc[1];
    pCg = pSrc[2];

    dstPad = (dstStep - (width * 4));
    srcPad[0] = (srcStep[0] - (width * 2));
    srcPad[1] = (srcStep[1] - (width * 2));
    srcPad[2] = (srcStep[2] - (width * 2));

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            Y = *pY;
            Co = *pCo;
            Cg = *pCg;

            pY++;
            pCo++;
            pCg++;

            Y += 128;

            t = Y - (Cg >> 1);
            G = Cg + t;
            B = t - (Co >> 1);
            R = B + Co;

            CLAMP(R, 0, 255);
            CLAMP(G, 0, 255);
            CLAMP(B, 0, 255);

            pRGB[0] = (uint8_t) B;
            pRGB[1] = (uint8_t) G;
            pRGB[2] = (uint8_t) R;
            pRGB[3] = 0xFF;
            pRGB += 4;
        }

        pRGB += dstPad;
        pY = (int16_t*) (((uint8_t*) pY) + srcPad[0]);
        pCo = (int16_t*) (((uint8_t*) pCo) + srcPad[1]);
        pCg = (int16_t*) (((uint8_t*) pCg) + srcPad[2]);
    }

    return XppSuccess;
}

/**
 * YCoCgR420
 */

XppStatus Xpp_YCoCgR420ToRGB_8u_P3AC4R_generic(const uint8_t* pSrc[3], uint32_t srcStep[3],
    uint8_t* pDst, uint32_t dstStep, uint32_t width, uint32_t height)
{
    uint32_t x, y;
    uint32_t dstPad;
    uint32_t srcPad[3];
    uint32_t fullWidth;
    uint32_t fullHeight;
    uint32_t halfWidth;
    uint32_t halfHeight;
    const uint8_t* pY;
    const uint8_t* pCo;
    const uint8_t* pCg;
    int32_t R[4];
    int32_t G[4];
    int32_t B[4];
    int32_t Y[4];
    int32_t Co, Cg, t[4];
    uint8_t* pRGB = pDst;

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

    for (y = 0; y < halfHeight; y++)
    {
        for (x = 0; x < halfWidth; x++)
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
        }

        pY = (uint8_t*) (((uint8_t*) pY) + srcPad[0] + srcStep[0]);
        pCo = (uint8_t*) (((uint8_t*) pCo) + srcPad[1]);
        pCg = (uint8_t*) (((uint8_t*) pCg) + srcPad[2]);
        pRGB = pRGB + dstPad + dstStep;
    }

    return XppSuccess;
}

XppStatus Xpp_RGBToYCoCgR420_8u_P3AC4R_generic(const uint8_t* pSrc, uint32_t srcStep, uint8_t* pDst[3], uint32_t dstStep[3],
                    uint32_t width, uint32_t height)
{
    uint32_t x, y;
    uint32_t srcPad;
    uint32_t dstPad[3];
    uint32_t fullWidth;
    uint32_t fullHeight;
    uint32_t halfWidth;
    uint32_t halfHeight;
    uint8_t* pY;
    uint8_t* pCo;
    uint8_t* pCg;
    int32_t R[4];
    int32_t G[4];
    int32_t B[4];
    int32_t Y[4];
    int32_t Co[4];
    int32_t Cg[4];
    int32_t t[4];
    int32_t sCo, sCg;
    const uint8_t* pRGB = pSrc;

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

    for (y = 0; y < halfHeight; y++)
    {
        for (x = 0; x < halfWidth; x++)
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
        }

        pRGB = pRGB + srcPad + srcStep;
        pY = pY + dstPad[0] + dstStep[0];
        pCo = pCo + dstPad[1];
        pCg = pCg + dstPad[2];
    }

    return XppSuccess;
}

XppStatus Xpp_RGBToYCoCgR_16s_P3AC4R(const uint8_t* pSrc, uint32_t srcStep,
                     int16_t* pDst[3], uint32_t dstStep[3], uint32_t width, uint32_t height)
{
    XppPrimitives* primitives = XppPrimitives_Get();
    return primitives->RGBToYCoCgR_16s_P3AC4R(pSrc, srcStep, pDst, dstStep, width, height);
}

XppStatus Xpp_YCoCgRToRGB_16s_P3AC4R(const int16_t* pSrc[3], uint32_t srcStep[3],
                     uint8_t* pDst, uint32_t dstStep, uint32_t width, uint32_t height)
{
    XppPrimitives* primitives = XppPrimitives_Get();
    return primitives->YCoCgRToRGB_16s_P3AC4R(pSrc, srcStep, pDst, dstStep, width, height);
}

XppStatus Xpp_YCoCgR420ToRGB_8u_P3AC4R(const uint8_t* pSrc[3], uint32_t srcStep[3], uint8_t* pDst, uint32_t dstStep,
    uint32_t width, uint32_t height)
{
    XppPrimitives* primitives = XppPrimitives_Get();
    return primitives->YCoCgR420ToRGB_8u_P3AC4R(pSrc, srcStep, pDst, dstStep, width, height);
}

XppStatus Xpp_RGBToYCoCgR420_8u_P3AC4R(const uint8_t* pSrc, uint32_t srcStep, uint8_t* pDst[3], uint32_t dstStep[3],
    uint32_t width, uint32_t height)
{
    XppPrimitives* primitives = XppPrimitives_Get();
    return primitives->RGBToYCoCgR420_8u_P3AC4R(pSrc, srcStep, pDst, dstStep, width, height);
}
