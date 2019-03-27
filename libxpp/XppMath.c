
#include <xpp/math.h>

void Xpp_MulC_16s_I(int16_t val, int16_t* pSrcDst, int len)
{
	int16_t tmp;

	while (len--)
	{
		tmp = *pSrcDst * val;
		*pSrcDst++ = tmp;
	}
}

void Xpp_DivC_16s_I(int16_t val, int16_t* pSrcDst, int len)
{
	int16_t tmp;

	while (len--)
	{
		tmp = *pSrcDst / val;
		*pSrcDst++ = tmp;
	}
}
