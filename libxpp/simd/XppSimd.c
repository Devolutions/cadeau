
#include <xpp/platform.h>

#include <xpp/math.h>

#define XPP_SIMD_INTERNAL
#include "XppSimd.h"

#if defined(_M_IX86) || defined(_M_AMD64)
#include "XppYCoCgR_sse2.c"
#include "XppCopy_sse2.c"
#include "XppCompare_sse2.c"
#include "XppSimd_x86.c"
#endif

#if defined(_M_ARM) || defined(_M_ARM64)
#include "XppYCoCgR_neon.c"
#include "XppCopy_neon.c"
#include "XppCompare_neon.c"
#include "XppSimd_arm.c"
#endif

#include <xpp/simd.h>

uint32_t Xpp_SimdInit(void)
{
    return init_simd();
}

uint32_t Xpp_SimdGet(void)
{
    return get_simd();
}

uint32_t Xpp_SimdAuto(void)
{
    return auto_simd();
}

uint32_t Xpp_SimdSet(uint32_t simd)
{
    return override_simd(simd);
}
