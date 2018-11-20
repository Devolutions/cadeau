
#include <xpp/simd.h>

#ifdef WITH_SIMD
#include "simd/simd.h"
#endif

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
