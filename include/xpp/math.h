#ifndef XPP_MATH_H
#define XPP_MATH_H

#include <xpp/xpp.h>

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef min
#define min(_a, _b)     ((_a < _b) ? _a : _b)
#endif

#ifndef max
#define max(_a, _b)     ((_a > _b) ? _a : _b)
#endif

#ifndef abs
#define abs(_a)         (((_a) < 0) ? -(_a) : (_a))
#endif

// clamp return value

#ifndef clamp
#define clamp(_val, _min, _max) (((_val) > (_max)) ? (_max) : (((_val) < (_min)) ? (_min) : (_val)))
#endif

// clamp value in-place

#define CLAMP(_val, _min, _max) \
    if (_val < _min) _val = _min; \
    else if (_val > _max) _val = _max;

XPP_EXPORT XppStatus Xpp_MulC_16s_I(int16_t val, int16_t* pSrcDst, int32_t len);

XPP_EXPORT XppStatus Xpp_DivC_16s_I(int16_t val, int16_t* pSrcDst, int32_t len);

#ifdef __cplusplus
}
#endif

#endif /* XPP_MATH_H */
