#ifndef XPP_COPY_H
#define XPP_COPY_H

#include <xpp/xpp.h>

#ifdef __cplusplus
extern "C" {
#endif

XPP_EXPORT int Xpp_Copy(uint8_t* pDstData, int nDstStep, int nXDst, int nYDst, int nWidth, int nHeight,
			      uint8_t* pSrcData, int nSrcStep, int nXSrc, int nYSrc);

XPP_EXPORT int Xpp_CopyFromRetina(uint8_t* pDstData, int nDstStep, int nXDst, int nYDst, int nWidth, int nHeight,
					uint8_t* pSrcData, int nSrcStep, int nXSrc, int nYSrc);

XPP_EXPORT int Xpp_Move(uint8_t* pData, int nStep, int nXDst, int nYDst, int nWidth, int nHeight, int nXSrc,
			      int nYSrc);

XPP_EXPORT int Xpp_Copy_c(uint8_t* pDstData, int nDstStep, int nXDst, int nYDst, int nWidth, int nHeight,
				uint8_t* pSrcData, int nSrcStep, int nXSrc, int nYSrc);

XPP_EXPORT int Xpp_CopyFromRetina_c(uint8_t* pDstData, int nDstStep, int nXDst, int nYDst, int nWidth,
					  int nHeight, uint8_t* pSrcData, int nSrcStep, int nXSrc, int nYSrc);

XPP_EXPORT int Xpp_Move_c(uint8_t* pData, int nStep, int nXDst, int nYDst, int nWidth, int nHeight, int nXSrc,
				int nYSrc);

XPP_EXPORT int Xpp_Copy_halide(uint8_t* pDstData, int nDstStep, int nXDst, int nYDst, int nWidth, int nHeight,
				     uint8_t* pSrcData, int nSrcStep, int nXSrc, int nYSrc);

#ifdef __cplusplus
}
#endif

#endif /* XPP_COPY_H */
