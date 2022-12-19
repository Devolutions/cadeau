#ifndef XPP_SIMD_H
#define XPP_SIMD_H

#include <xpp/xpp.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XPP_SIMD_NONE       0
#define XPP_SIMD_SSE2       1
#define XPP_SIMD_NEON       2

XPP_EXPORT uint32_t Xpp_SimdInit(void);
XPP_EXPORT uint32_t Xpp_SimdGet(void);
XPP_EXPORT uint32_t Xpp_SimdAuto(void);
XPP_EXPORT uint32_t Xpp_SimdSet(uint32_t simd);

#ifdef __cplusplus
}
#endif

#endif /* XPP_SIMD_H */
