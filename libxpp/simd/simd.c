
#include <xpp/platform.h>

#include <xpp/math.h>

#define XPP_SIMD_INTERNAL
#include "simd.h"

#if defined(_M_IX86) || defined(_M_AMD64)
#include "XppCodec_sse2.c"
#include "XppColor_sse2.c"
#include "simd_x86.c"
#endif

#if defined(_M_ARM) || defined(_M_ARM64)
#include "XppCodec_neon.c"
#include "XppColor_neon.c"
#include "simd_arm.c"
#endif
