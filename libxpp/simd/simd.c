
#include <xpp/platform.h>

#include <xpp/math.h>

#define XPP_SIMD_INTERNAL
#include "simd.h"

#if defined(_M_IX86) || defined(_M_AMD64)
#include "XppYCoCgR_sse2.c"
#include "XppCopy_sse2.c"
#include "XppCompare_sse2.c"
#include "simd_x86.c"
#endif

#if defined(_M_ARM) || defined(_M_ARM64)
#include "XppYCoCgR_neon.c"
#include "XppCopy_neon.c"
#include "XppCompare_neon.c"
#include "simd_arm.c"
#endif
