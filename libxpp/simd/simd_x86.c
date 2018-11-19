
#ifdef _WIN32
#include <intrin.h>
#else
#include <cpuid.h>
#endif

#include <stdlib.h>

static unsigned int simd_type = ~0;

#ifdef _WIN32

#define CPUID(function, eax, ebx, ecx, edx) {  \
	int cpu_info[4];  \
	__cpuid(cpu_info, function);  \
	eax = (unsigned int)cpu_info[0];  \
	ebx = (unsigned int)cpu_info[1];  \
	ecx = (unsigned int)cpu_info[2];  \
	edx = (unsigned int)cpu_info[3];  \
}

#else

#define CPUID(function, eax, ebx, ecx, edx)  \
	__get_cpuid(1, &eax, &ebx, &ecx, &edx)

#endif

unsigned int get_simd(void)
{
	return simd_type;
}

unsigned int override_simd(unsigned int simd)
{
	return simd_type = simd;
}

unsigned int auto_simd(void)
{
	simd_type = ~0U;
	return init_simd();
}

unsigned int init_simd(void)
{
#ifndef __x86_64__
	unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
#endif
	char *env = NULL;

	if (simd_type != ~0U)
		return simd_type;

#ifdef __x86_64__

	simd_type = SIMD_SSE2;

#else

	CPUID(1, eax, ebx, ecx, edx);
	if (edx & (1U << 26))
		simd_type = SIMD_SSE2;

#endif

	env = getenv("SIMD_FORCENONE");
	if ((env != NULL) && (strcmp(env, "1") == 0))
		simd_type = SIMD_NONE;

	return simd_type;
}
