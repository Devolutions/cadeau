#ifndef XPP_GENERIC_INTERNAL_H
#define XPP_GENERIC_INTERNAL_H

#include <xpp/xpp.h>

#ifdef __cplusplus
extern "C" {
#endif

XPP_EXPORT XppStatus Xpp_Compare32_generic(uint8_t* pData1, int step1, uint8_t* pData2, int step2, int width, int height,
				     XppRect* rect);

XPP_EXPORT XppStatus Xpp_Compare8_generic(uint8_t* pData1, int step1, uint8_t* pData2, int step2, int width, int height,
				    XppRect* rect);

XPP_EXPORT XppStatus Xpp_Copy_generic(uint8_t* pDstData, int nDstStep, int nXDst, int nYDst, int nWidth, int nHeight,
				uint8_t* pSrcData, int nSrcStep, int nXSrc, int nYSrc);

XPP_EXPORT XppStatus Xpp_Move_generic(uint8_t* pData, int nStep, int nXDst, int nYDst, int nWidth, int nHeight, int nXSrc,
				int nYSrc);

XPP_EXPORT XppStatus Xpp_YCoCgRToRGB_16s_P3AC4R_generic(const int16_t* pSrc[3], uint32_t srcStep[3],
						uint8_t* pDst, uint32_t dstStep, uint32_t width, uint32_t height);

XPP_EXPORT XppStatus Xpp_RGBToYCoCgR_16s_P3AC4R_generic(const uint8_t* pSrc, uint32_t srcStep,
						int16_t* pDst[3], uint32_t dstStep[3], uint32_t width, uint32_t height);

XPP_EXPORT XppStatus Xpp_YCoCgR420ToRGB_8u_P3AC4R_generic(const uint8_t* pSrc[3], uint32_t srcStep[3], uint8_t* pDst, uint32_t dstStep,
						    uint32_t width, uint32_t height);

XPP_EXPORT XppStatus Xpp_RGBToYCoCgR420_8u_P3AC4R_generic(const uint8_t* pSrc, uint32_t srcStep, uint8_t* pDst[3],
						    uint32_t dstStep[3], uint32_t width, uint32_t height);

#ifdef __cplusplus
}
#endif

#endif /* XPP_GENERIC_INTERNAL_H */
