#ifndef XPP_COLOR_H
#define XPP_COLOR_H

#include <xpp/xpp.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Alpha
 */

XPP_EXPORT XppStatus Xpp_MultiplyAlpha(const uint8_t* pSrc, uint32_t srcStep,
			    uint8_t* pDst, uint32_t dstStep, uint32_t width, uint32_t height);

XPP_EXPORT XppStatus Xpp_UnmultiplyAlpha(const uint8_t* pSrc, uint32_t srcStep,
			      uint8_t* pDst, uint32_t dstStep, uint32_t width, uint32_t height);

/**
 * A710
 */

XPP_EXPORT XppStatus Xpp_RGBToA710_16s_P3AC4R(const uint8_t* pSrc, uint32_t srcStep,
			      int16_t* pDst[3], uint32_t dstStep[3], uint32_t width, uint32_t height);

XPP_EXPORT XppStatus Xpp_A710ToRGB_16s_P3AC4R(const int16_t* pSrc[3], uint32_t srcStep[3],
			      uint8_t* pDst, uint32_t dstStep, uint32_t width, uint32_t height);

/**
 * YCbCr
 */

XPP_EXPORT XppStatus Xpp_YCbCr420ToRGB_8u_P3AC4R(const uint8_t* pSrc[3], uint32_t srcStep[3],
				    uint8_t* pDst, uint32_t dstStep, uint32_t width, uint32_t height);

XPP_EXPORT XppStatus Xpp_RGBToYCbCr420_8u_P3AC4R(const uint8_t* pSrc, uint32_t srcStep,
				    uint8_t* pDst[3], uint32_t dstStep[3], uint32_t width, uint32_t height);

/**
 * YCoCg-R
 */

XPP_EXPORT XppStatus Xpp_YCoCgRToRGB_16s_P3AC4R(const int16_t* pSrc[3], uint32_t srcStep[3],
					   uint8_t* pDst, uint32_t dstStep, uint32_t width, uint32_t height);

XPP_EXPORT XppStatus Xpp_RGBToYCoCgR_16s_P3AC4R(const uint8_t* pSrc, uint32_t srcStep,
					   int16_t* pDst[3], uint32_t dstStep[3], uint32_t width, uint32_t height);

XPP_EXPORT XppStatus Xpp_YCoCgR420ToRGB_8u_P3AC4R(const uint8_t* pSrc[3], uint32_t srcStep[3], uint8_t* pDst, uint32_t dstStep,
					     uint32_t width, uint32_t height);

XPP_EXPORT XppStatus Xpp_RGBToYCoCgR420_8u_P3AC4R(const uint8_t* pSrc, uint32_t srcStep, uint8_t* pDst[3],
					     uint32_t dstStep[3], uint32_t width, uint32_t height);

#ifdef __cplusplus
}
#endif

#endif /* XPP_COLOR_H */
