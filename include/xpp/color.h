#ifndef XPP_COLOR_H
#define XPP_COLOR_H

#include <xpp/xpp.h>

#define XPP_COLOR_FORMAT_XRGB		0
#define XPP_COLOR_FORMAT_YUV420		1
#define XPP_COLOR_FORMAT_YCOCGR		2
#define XPP_COLOR_FORMAT_YCOCGR420	3

#ifdef __cplusplus
extern "C" {
#endif

XPP_EXPORT void Xpp_YCoCgR420ToRGB_8u_P3AC4R(const uint8_t* pSrc[3], int srcStep[3], uint8_t* pDst, int dstStep,
						   int width, int height);

XPP_EXPORT void Xpp_RGBToYCoCgR420_8u_P3AC4R(const uint8_t* pSrc, int32_t srcStep, uint8_t* pDst[3],
						   int32_t dstStep[3], int width, int height);

XPP_EXPORT void Xpp_RGBToYCoCgR420_8u_P3AC4R_ds2x(const uint8_t* pSrc, int32_t srcStep, uint8_t* pDst[3],
							int32_t dstStep[3], int width, int height);

XPP_EXPORT void Xpp_YCoCgR420ToRGB_8u_P3AC4R_c(const uint8_t* pSrc[3], int srcStep[3], uint8_t* pDst, int dstStep,
						     int width, int height);

XPP_EXPORT void Xpp_RGBToYCoCgR420_8u_P3AC4R_c(const uint8_t* pSrc, int32_t srcStep, uint8_t* pDst[3],
						     int32_t dstStep[3], int width, int height);

XPP_EXPORT void Xpp_Halide_RGBToYCoCgR420_8u_P3AC4R(const uint8_t* pSrc, int32_t srcStep, uint8_t* pDst[3],
							  int32_t dstStep[3], int width, int height);

XPP_EXPORT void Xpp_Halide_YCoCgR420ToRGB_8u_P3AC4R(const uint8_t* pSrc[3], int srcStep[3], uint8_t* pDst,
							  int dstStep, int width, int height);

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

XPP_EXPORT XppStatus Xpp_RGBToYCoCgR_16s_P3AC4R(const uint8_t* pSrc, int32_t srcStep,
					   int16_t* pDst[3], int32_t dstStep[3], uint32_t width, uint32_t height);

#ifdef __cplusplus
}
#endif

#endif /* XPP_COLOR_H */
