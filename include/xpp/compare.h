#ifndef XPP_COMPARE_H
#define XPP_COMPARE_H

#include <xpp/xpp.h>

#ifdef __cplusplus
extern "C" {
#endif

XPP_EXPORT XppStatus Xpp_Compare(uint8_t* pData1, int nStep1, int nWidth, int nHeight, uint8_t* pData2, int nStep2,
				 XppRect* rect);

XPP_EXPORT XppStatus Xpp_Compare32(uint8_t* pData1, int step1, uint8_t* pData2, int step2, int width, int height,
				   XppRect* rect);

XPP_EXPORT XppStatus Xpp_Compare8(uint8_t* pData1, int step1, uint8_t* pData2, int step2, int width, int height,
				  XppRect* rect);

XPP_EXPORT XppStatus Xpp_Compare_c(uint8_t* pData1, int nStep1, int nWidth, int nHeight, uint8_t* pData2, int nStep2,
				   XppRect* rect);

XPP_EXPORT XppStatus Xpp_Compare32_c(uint8_t* pData1, int step1, uint8_t* pData2, int step2, int width, int height,
				     XppRect* rect);

XPP_EXPORT XppStatus Xpp_Compare8_c(uint8_t* pData1, int step1, uint8_t* pData2, int step2, int width, int height,
				    XppRect* rect);

#ifdef __cplusplus
}
#endif

#endif /* XPP_COMPARE_H */
