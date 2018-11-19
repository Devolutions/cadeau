
#include <xpp/prim.h>
#include <xpp/codec.h>
#include <xpp/color.h>

#ifdef WITH_SIMD
#include "simd/simd.h"
#endif

static XppPrimitives g_Primitives = { 0 };
static bool primitivesInitialized = false;

bool XppPrimitives_Init(XppPrimitives* primitives, uint32_t flags)
{
	bool result = false;

	if (!primitives)
	{
		return false;
	}

	memset(primitives, 0, sizeof(XppPrimitives));

	if (flags & XPP_PRIMITIVES_GENERIC)
	{
		primitives->Compare32 = Xpp_Compare32_c;
		primitives->Compare8 = Xpp_Compare8_c;
		primitives->Copy = Xpp_Copy_c;
		primitives->CopyFromRetina = Xpp_CopyFromRetina_c;
		primitives->Move = Xpp_Move_c;
		primitives->RGBToYCoCgR420_8u_P3AC4R = Xpp_RGBToYCoCgR420_8u_P3AC4R_c;
		primitives->YCoCgR420ToRGB_8u_P3AC4R = Xpp_YCoCgR420ToRGB_8u_P3AC4R_c;

		result = true;
	}

#ifdef WITH_SIMD
	if (flags & XPP_PRIMITIVES_SIMD)
	{
		primitives->Compare32 = Xpp_Compare32_simd;
		primitives->Compare8 = Xpp_Compare8_simd;
		primitives->Copy = Xpp_Copy_simd;
		primitives->CopyFromRetina = Xpp_CopyFromRetina_simd;
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
		primitives->Compare32 = Xpp_Compare32_halide;
		primitives->RGBToYCoCgR420_8u_P3AC4R = Xpp_Halide_RGBToYCoCgR420_8u_P3AC4R;
		primitives->YCoCgR420ToRGB_8u_P3AC4R = Xpp_Halide_YCoCgR420ToRGB_8u_P3AC4R;
		result = true;
	}
#endif

	return result;
}

XppPrimitives* XppPrimitives_Get()
{
	if (!primitivesInitialized)
		XppPrimitives_Init(&g_Primitives, XPP_PRIMITIVES_ALL);

#ifdef WITH_HALIDE
#ifdef WITH_SIMD
	g_Primitives.Compare32 = Xpp_Compare32_simd;
	g_Primitives.Compare8 = Xpp_Compare8_simd;
#else
	g_Primitives.Compare32 = Xpp_Compare32_c;
	g_Primitives.Compare8 = Xpp_Compare8_c;
#endif
#endif

	return &g_Primitives;
}
