#ifndef XPP_COPY_H
#define XPP_COPY_H

#include <xpp/xpp.h>

#ifdef __cplusplus
extern "C" {
#endif

XPP_EXPORT XppStatus Xpp_Copy(uint8_t* pDstData, int32_t nDstStep, int32_t nXDst, int32_t nYDst, int32_t nWidth, int32_t nHeight,
                    uint8_t* pSrcData, int32_t nSrcStep, int32_t nXSrc, int32_t nYSrc);

XPP_EXPORT XppStatus Xpp_Move(uint8_t* pData, int32_t nStep, int32_t nXDst, int32_t nYDst, int32_t nWidth, int32_t nHeight,
                    int32_t nXSrc, int32_t nYSrc);

#ifdef __cplusplus
}
#endif

#endif /* XPP_COPY_H */
