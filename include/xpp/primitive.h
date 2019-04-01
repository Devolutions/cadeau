#ifndef XPP_PRIMITIVE_H
#define XPP_PRIMITIVE_H

#include <xpp/xpp.h>

#include <xpp/simd.h>
#include <xpp/copy.h>
#include <xpp/compare.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XPP_PRIMITIVES_GENERIC		0x1
#define XPP_PRIMITIVES_SIMD		0x2
#define XPP_PRIMITIVES_HALIDE		0x4
#define XPP_PRIMITIVES_ALL		0x7

typedef XppStatus (*fnXpp_Compare32)(uint8_t* pData1, int step1, uint8_t* pData2, int step2, int width, int height,
				       XppRect* rect);

typedef XppStatus (*fnXpp_Compare8)(uint8_t* pData1, int step1, uint8_t* pData2, int step2, int width, int height,
				      XppRect* rect);

typedef XppStatus (*fnXpp_Copy)(uint8_t* pDstData, int nDstStep, int nXDst, int nYDst, int nWidth, int nHeight,
				  uint8_t* pSrcData, int nSrcStep, int nXSrc, int nYSrc);

typedef XppStatus (*fnXpp_Move)(uint8_t* pData, int nStep, int nXDst, int nYDst, int nWidth, int nHeight, int nXSrc,
				  int nYSrc);

typedef XppStatus (*fnXpp_YCoCgRToRGB_16s_P3AC4R)(const int16_t* pSrc[3], uint32_t srcStep[3],
						uint8_t* pDst, uint32_t dstStep, uint32_t width, uint32_t height);

typedef XppStatus (*fnXpp_RGBToYCoCgR_16s_P3AC4R)(const uint8_t* pSrc, uint32_t srcStep,
						int16_t* pDst[3], uint32_t dstStep[3], uint32_t width, uint32_t height);

typedef XppStatus (*fnXpp_YCoCgR420ToRGB_8u_P3AC4R)(const uint8_t* pSrc[3], uint32_t srcStep[3], uint8_t* pDst,
						       uint32_t dstStep, uint32_t width, uint32_t height);

typedef XppStatus (*fnXpp_RGBToYCoCgR420_8u_P3AC4R)(const uint8_t* pSrc, uint32_t srcStep, uint8_t* pDst[3],
						       uint32_t dstStep[3], uint32_t width, uint32_t height);

typedef struct now_primitives
{
	fnXpp_Compare32 Compare32;
	fnXpp_Compare8 Compare8;
	fnXpp_Copy Copy;
	fnXpp_Move Move;
	fnXpp_YCoCgRToRGB_16s_P3AC4R YCoCgRToRGB_16s_P3AC4R;
	fnXpp_RGBToYCoCgR_16s_P3AC4R RGBToYCoCgR_16s_P3AC4R;
	fnXpp_YCoCgR420ToRGB_8u_P3AC4R YCoCgR420ToRGB_8u_P3AC4R;
	fnXpp_RGBToYCoCgR420_8u_P3AC4R RGBToYCoCgR420_8u_P3AC4R;
} XppPrimitives;

XPP_EXPORT bool XppPrimitives_Init(XppPrimitives* primitives, uint32_t flags);
XPP_EXPORT XppPrimitives* XppPrimitives_Get();

#ifdef __cplusplus
}
#endif

#endif /* XPP_PRIMITIVE_H */
