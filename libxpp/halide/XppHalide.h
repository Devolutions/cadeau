#ifndef XPP_HALIDE_INTERNAL_H
#define XPP_HALIDE_INTERNAL_H

#include <xpp/xpp.h>

#include <HalideRuntime.h>

#define HALIDE_BUFFER_DEFINE(_x) \
	halide_buffer_t _x; \
	halide_dimension_t _x ## _dim[4]; \
	_x.dim = (halide_dimension_t*) & _x ## _dim

void halide_setup_rgb_buffer_t(halide_buffer_t* buffer, uint8_t* data, int width, int height, int stride);
void halide_setup_ycocg_buffer_t(halide_buffer_t* buffer, uint8_t* data, int width, int height, int stride);
void halide_setup_u32_buffer_t(halide_buffer_t* buffer, uint32_t* data, int width, int height, int stride);
void halide_setup_1d_u32_buffer_t(halide_buffer_t* buffer, uint32_t* data, int width);
void halide_setup_u8_buffer_t(halide_buffer_t* buffer, uint8_t* data, int width, int height, int stride);
void halide_setup_1d_u8_buffer_t(halide_buffer_t* buffer, uint8_t* data, int width);

#ifdef __cplusplus
extern "C" {
#endif

XPP_EXPORT XppStatus Xpp_RGBToYCoCgR420_8u_P3AC4R_halide(const uint8_t* pSrc, uint32_t srcStep, uint8_t* pDst[3],
							 uint32_t dstStep[3], uint32_t width, uint32_t height);

XPP_EXPORT XppStatus Xpp_YCoCgR420ToRGB_8u_P3AC4R_halide(const uint8_t* pSrc[3], uint32_t srcStep[3], uint8_t* pDst, uint32_t dstStep,
							 uint32_t width, uint32_t height);

XPP_EXPORT XppStatus Xpp_Compare8_halide(uint8_t* pData1, int step1, uint8_t* pData2, int step2, int width, int height,
					 XppRect* rect);

XPP_EXPORT XppStatus Xpp_Compare32_halide(uint8_t* pData1, int step1, uint8_t* pData2, int step2, int width, int height,
					  XppRect* rect);

XPP_EXPORT XppStatus Xpp_Copy_halide(uint8_t* pDstData, int nDstStep, int nXDst, int nYDst, int nWidth, int nHeight,
				     uint8_t* pSrcData, int nSrcStep, int nXSrc, int nYSrc);

#ifdef __cplusplus
}
#endif

#endif /* XPP_HALIDE_INTERNAL_H */
