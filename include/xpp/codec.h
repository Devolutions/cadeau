#ifndef XPP_CODEC_H
#define XPP_CODEC_H

#include <xpp/xpp.h>

#ifdef __cplusplus
extern "C" {
#endif

XPP_EXPORT int Xpp_Copy(uint8_t* pDstData, int nDstStep, int nXDst, int nYDst, int nWidth, int nHeight,
			      uint8_t* pSrcData, int nSrcStep, int nXSrc, int nYSrc);

XPP_EXPORT int Xpp_CopyFromRetina(uint8_t* pDstData, int nDstStep, int nXDst, int nYDst, int nWidth, int nHeight,
					uint8_t* pSrcData, int nSrcStep, int nXSrc, int nYSrc);

XPP_EXPORT int Xpp_Compare(uint8_t* pData1, int nStep1, int nWidth, int nHeight, uint8_t* pData2, int nStep2,
				 XppRect* rect);

XPP_EXPORT int Xpp_Compare32(uint8_t* pData1, int step1, uint8_t* pData2, int step2, int width, int height,
				   XppRect* rect);

XPP_EXPORT int Xpp_Compare8(uint8_t* pData1, int step1, uint8_t* pData2, int step2, int width, int height,
				  XppRect* rect);

XPP_EXPORT int Xpp_Move(uint8_t* pData, int nStep, int nXDst, int nYDst, int nWidth, int nHeight, int nXSrc,
			      int nYSrc);

XPP_EXPORT int Xpp_Copy_c(uint8_t* pDstData, int nDstStep, int nXDst, int nYDst, int nWidth, int nHeight,
				uint8_t* pSrcData, int nSrcStep, int nXSrc, int nYSrc);

XPP_EXPORT int Xpp_CopyFromRetina_c(uint8_t* pDstData, int nDstStep, int nXDst, int nYDst, int nWidth,
					  int nHeight, uint8_t* pSrcData, int nSrcStep, int nXSrc, int nYSrc);

XPP_EXPORT int Xpp_Compare_c(uint8_t* pData1, int nStep1, int nWidth, int nHeight, uint8_t* pData2, int nStep2,
				   XppRect* rect);

XPP_EXPORT int Xpp_Compare32_c(uint8_t* pData1, int step1, uint8_t* pData2, int step2, int width, int height,
				     XppRect* rect);

XPP_EXPORT int Xpp_Compare8_c(uint8_t* pData1, int step1, uint8_t* pData2, int step2, int width, int height,
				    XppRect* rect);

XPP_EXPORT int Xpp_Move_c(uint8_t* pData, int nStep, int nXDst, int nYDst, int nWidth, int nHeight, int nXSrc,
				int nYSrc);

XPP_EXPORT int Xpp_Copy_halide(uint8_t* pDstData, int nDstStep, int nXDst, int nYDst, int nWidth, int nHeight,
				     uint8_t* pSrcData, int nSrcStep, int nXSrc, int nYSrc);

XPP_EXPORT int Xpp_Compare8_halide(uint8_t* pData1, int step1, uint8_t* pData2, int step2, int width, int height,
					 XppRect* rect);

XPP_EXPORT int Xpp_Compare32_halide(uint8_t* pData1, int step1, uint8_t* pData2, int step2, int width, int height,
					  XppRect* rect);

#ifdef __cplusplus
}
#endif

#endif /* XPP_CODEC_H */
