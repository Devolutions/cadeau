#ifndef XPP_COMPARE_H
#define XPP_COMPARE_H

#include <xpp/xpp.h>

#ifdef __cplusplus
extern "C" {
#endif

XPP_EXPORT XppStatus Xpp_Compare32(uint8_t* pData1, int32_t step1, uint8_t* pData2, int32_t step2,
                    int32_t width, int32_t height, XppRect* rect);

XPP_EXPORT XppStatus Xpp_Compare8(uint8_t* pData1, int32_t step1, uint8_t* pData2, int32_t step2,
                    int32_t width, int32_t height, XppRect* rect);

#ifdef __cplusplus
}
#endif

#endif /* XPP_COMPARE_H */
