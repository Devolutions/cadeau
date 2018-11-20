#ifndef XPP_SIMD_INTERNAL_H
#define XPP_SIMD_INTERNAL_H

#include <xpp/xpp.h>
#include <xpp/math.h>
#include <xpp/color.h>

#define SIMD_NONE	0
#define SIMD_SSE2	1
#define SIMD_NEON	2

#if defined(__aarch64__) || defined(__x86_64__)
#define BITS	64
#else
#define BITS	32
#endif

uint32_t init_simd(void);
uint32_t get_simd(void);
uint32_t auto_simd(void);
uint32_t override_simd(uint32_t simd);

#ifndef XPP_SIMD_INTERNAL

#if (defined(__arm__) || defined(__aarch64__)) && defined(__ANDROID__)

/* Using memcpy() to implement the Copy algorithm always seems to be faster on
   Android, but our algorithm is faster on iOS. */

#define Xpp_Copy_simd Xpp_Copy_c

#else

int Xpp_Copy_simd(uint8_t* pDstData, int nDstStep, int nXDst, int nYDst,
	int nWidth, int nHeight, uint8_t* pSrcData, int nSrcStep, int nXSrc, int nYSrc);

#endif

int Xpp_Move_simd(uint8_t* pData, int nStep, int nXDst, int nYDst,
	int nWidth, int nHeight, int nXSrc, int nYSrc);

int Xpp_CopyFromRetina_simd(uint8_t* pDstData, int nDstStep, int nXDst,
	int nYDst, int nWidth, int nHeight, uint8_t* pSrcData, int nSrcStep, int nXSrc,
	int nYSrc);

#if !defined(LINEAR_COMPARE32) && (defined(__arm__) || defined(__aarch64__)) && defined(__ANDROID__)

#define Xpp_Compare32_simd Xpp_Compare32_c

#else

int Xpp_Compare32_simd(uint8_t* pData1, int step1, uint8_t* pData2, int step2,
	int width, int height, XppRect* rect);

#endif

#if defined(__aarch64__) && defined(__APPLE__)

/* Our optimized uint32_t C Compare8 algorithm is faster than NEON when using
   64-bit code on iOS devices. */

#define Xpp_Compare8_simd Xpp_Compare8_c

#else

int Xpp_Compare8_simd(uint8_t* pData1, int step1, uint8_t* pData2, int step2,
	int width, int height, XppRect* rect);

#endif

void Xpp_YCoCgR420ToRGB_8u_P3AC4R_simd(const uint8_t* pSrc[3],
	int srcStep[3], uint8_t* pDst, int dstStep, int width, int height);

void Xpp_RGBToYCoCgR420_8u_P3AC4R_simd(const uint8_t* pSrc, int32_t srcStep,
	uint8_t* pDst[3], int32_t dstStep[3], int width, int height);

#endif

#endif /* XPP_SIMD_INTERNAL_H */
