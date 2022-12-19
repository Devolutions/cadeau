
#include <xpp/primitive.h>

#ifdef WITH_SIMD
#include "simd/XppSimd.h"
#endif

#ifdef WITH_HALIDE
#include "halide/XppHalide.h"
#endif

#include "XppGeneric.h"

static bool g_Initialized = false;
static XppPrimitives g_Primitives = { 0 };

bool XppPrimitives_Init(XppPrimitives* primitives, uint32_t flags)
{
    bool result = false;

    if (!primitives)
        return false;

    Xpp_SimdInit();

    xpp_memset(primitives, 0, sizeof(XppPrimitives));

    if (flags & XPP_PRIMITIVES_GENERIC)
    {
        primitives->Compare32 = Xpp_Compare32_generic;
        primitives->Compare8 = Xpp_Compare8_generic;
        primitives->Copy = Xpp_Copy_generic;
        primitives->Move = Xpp_Move_generic;
        primitives->RGBToYCoCgR_16s_P3AC4R = Xpp_RGBToYCoCgR_16s_P3AC4R_generic;
        primitives->YCoCgRToRGB_16s_P3AC4R = Xpp_YCoCgRToRGB_16s_P3AC4R_generic;
        primitives->RGBToYCoCgR420_8u_P3AC4R = Xpp_RGBToYCoCgR420_8u_P3AC4R_generic;
        primitives->YCoCgR420ToRGB_8u_P3AC4R = Xpp_YCoCgR420ToRGB_8u_P3AC4R_generic;
        result = true;
    }

#ifdef WITH_SIMD
    if (flags & XPP_PRIMITIVES_SIMD)
    {
        primitives->Compare32 = Xpp_Compare32_simd;
        primitives->Compare8 = Xpp_Compare8_simd;
        primitives->Copy = Xpp_Copy_simd;
        primitives->Move = Xpp_Move_simd;
        primitives->RGBToYCoCgR420_8u_P3AC4R = Xpp_RGBToYCoCgR420_8u_P3AC4R_simd;
        primitives->YCoCgR420ToRGB_8u_P3AC4R = Xpp_YCoCgR420ToRGB_8u_P3AC4R_simd;
        result = true;
    }
#endif

#ifdef WITH_HALIDE
    if (flags & XPP_PRIMITIVES_HALIDE)
    {
        primitives->Copy = Xpp_Copy_halide;
        //primitives->Compare32 = Xpp_Compare32_halide;
        //primitives->Compare8 = Xpp_Compare8_halide;
        primitives->RGBToYCoCgR_16s_P3AC4R = Xpp_RGBToYCoCgR_16s_P3AC4R_halide;
        primitives->YCoCgRToRGB_16s_P3AC4R = Xpp_YCoCgRToRGB_16s_P3AC4R_halide;
        primitives->RGBToYCoCgR420_8u_P3AC4R = Xpp_RGBToYCoCgR420_8u_P3AC4R_halide;
        primitives->YCoCgR420ToRGB_8u_P3AC4R = Xpp_YCoCgR420ToRGB_8u_P3AC4R_halide;
        result = true;
    }
#endif

    return result;
}

XppPrimitives* XppPrimitives_Get()
{
    if (!g_Initialized)
        XppPrimitives_Init(&g_Primitives, XPP_PRIMITIVES_ALL);

    g_Initialized = true;

    return &g_Primitives;
}
