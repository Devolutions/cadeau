#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <xpp/math.h>
#include <xpp/color.h>
#include <xpp/primitive.h>

#include <xpp/copy.h>

XppStatus Xpp_Copy_generic(uint8_t* pDstData, int32_t nDstStep, int32_t nXDst, int32_t nYDst, int32_t nWidth, int32_t nHeight, uint8_t* pSrcData,
            int nSrcStep, int32_t nXSrc, int32_t nYSrc)
{
    int32_t y;
    uint8_t* pSrcPixel;
    uint8_t* pDstPixel;

    if (nSrcStep < 0)
        nSrcStep = 4 * nWidth;

    if (nDstStep < 0)
        nDstStep = 4 * nWidth;

    pSrcPixel = &pSrcData[(nYSrc * nSrcStep) + (nXSrc * 4)];
    pDstPixel = &pDstData[(nYDst * nDstStep) + (nXDst * 4)];

    for (y = 0; y < nHeight; y++)
    {
        xpp_memcpy(pDstPixel, pSrcPixel, nWidth * 4);
        pSrcPixel = &pSrcPixel[nSrcStep];
        pDstPixel = &pDstPixel[nDstStep];
    }

    return XppSuccess;
}

XppStatus Xpp_Copy(uint8_t* pDstData, int32_t nDstStep, int32_t nXDst, int32_t nYDst, int32_t nWidth, int32_t nHeight, uint8_t* pSrcData,
          int32_t nSrcStep, int32_t nXSrc, int32_t nYSrc)
{
    XppPrimitives* primitives = XppPrimitives_Get();
    return primitives->Copy(pDstData, nDstStep, nXDst, nYDst, nWidth, nHeight, pSrcData, nSrcStep, nXSrc, nYSrc);
}

XppStatus Xpp_Move_generic(uint8_t* pData, int32_t nStep, int32_t nXDst, int32_t nYDst,
        int32_t nWidth, int32_t nHeight, int32_t nXSrc, int32_t nYSrc)
{
    uint8_t* pSrcPixel;
    uint8_t* pDstPixel;

    if (nStep < 0)
        nStep = 4 * nWidth;

    pSrcPixel = &pData[((nYSrc + nHeight - 1) * nStep) + (nXSrc * 4)];
    pDstPixel = &pData[((nYDst + nHeight - 1) * nStep) + (nXDst * 4)];

    if (pSrcPixel > pDstPixel)
        return Xpp_Copy_generic(pData, nStep, nXDst, nYDst, nWidth, nHeight, pData,
            nStep, nXSrc, nYSrc);

    while (nHeight--)
    {
        memmove(pDstPixel, pSrcPixel, nWidth * 4);
        pSrcPixel -= nStep;
        pDstPixel -= nStep;
    }

    return XppSuccess;
}

XppStatus Xpp_Move(uint8_t* pData, int32_t nStep, int32_t nXDst, int32_t nYDst, int32_t nWidth, int32_t nHeight, int32_t nXSrc, int32_t nYSrc)
{
    XppPrimitives* primitives = XppPrimitives_Get();
    return primitives->Move(pData, nStep, nXDst, nYDst, nWidth, nHeight, nXSrc, nYSrc);
}
