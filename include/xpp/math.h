#ifndef XPP_MATH_H
#define XPP_MATH_H

#include <xpp/xpp.h>

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef min
#define min(_a, _b)		((_a < _b) ? _a : _b)
#endif

#ifndef max
#define max(_a, _b)		((_a > _b) ? _a : _b)
#endif

#ifndef abs
#define abs(_a)			(((_a) < 0) ? -(_a) : (_a))
#endif

#ifndef clamp
#define clamp(_x, _lo, _hi)	(((_x) > (_hi)) ? (_hi) : (((_x) < (_lo)) ? (_lo) : (_x)))
#endif

#ifdef __cplusplus
}
#endif

#endif /* XPP_MATH_H */
