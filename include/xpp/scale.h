#ifndef XPP_SCALE_H
#define XPP_SCALE_H

#include <xpp/xpp.h>

#ifdef __cplusplus
extern "C" {
#endif

XPP_EXPORT XppStatus Xpp_Scale_8u_C4R(const uint8_t* pSrc, int srcStep, int srcWidth, int srcHeight, uint8_t* pDst,
				   int dstStep, int dstWidth, int dstHeight, int mode);

#ifdef __cplusplus
}
#endif

#endif /* XPP_SCALE_H */
