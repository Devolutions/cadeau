
#include <xpp/math.h>

/**
 * Arithmetic Functions:
 * https://software.intel.com/en-us/ipp-dev-reference-arithmetic-functions
 */

XppStatus Xpp_MulC_16s_I(int16_t val, int16_t* pSrcDst, int len)
{
	int16_t tmp;

	while (len--)
	{
		tmp = *pSrcDst * val;
		*pSrcDst++ = tmp;
	}

	return XppSuccess;
}

XppStatus Xpp_DivC_16s_I(int16_t val, int16_t* pSrcDst, int len)
{
	int16_t tmp;

	while (len--)
	{
		tmp = *pSrcDst / val;
		*pSrcDst++ = tmp;
	}

	return XppSuccess;
}
