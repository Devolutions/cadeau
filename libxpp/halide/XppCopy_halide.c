#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <xpp/copy.h>

#include <halide/Copy.h>
#include <halide/Compare32Stage1.h>
#include <halide/Compare8Stage1.h>
#include <HalideRuntime.h>

#include "XppHalide.h"

int Xpp_Copy_halide(uint8_t* pDstData, int nDstStep, int nXDst, int nYDst,
	int nWidth, int nHeight, uint8_t* pSrcData, int nSrcStep, int nXSrc, int nYSrc)
{
	HALIDE_BUFFER_DEFINE(input);
	HALIDE_BUFFER_DEFINE(output);

	halide_setup_u32_buffer_t(&input, (uint32_t*) &pSrcData[nXSrc * sizeof(uint32_t) + nYSrc * nSrcStep], nWidth,
				  nHeight, nSrcStep / sizeof(uint32_t));
	halide_setup_u32_buffer_t(&output, (uint32_t*) &pDstData[nXDst * sizeof(uint32_t) + nYDst * nDstStep], nWidth,
				  nHeight, nDstStep / sizeof(uint32_t));

	Copy(&input, &output);

	return 0;
}
