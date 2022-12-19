#ifndef XPP_H
#define XPP_H

/**
 * XPP: eXtreme Performance Primitives
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <limits.h>

#include <string.h>
#include <stdlib.h>

#include <stdio.h>
#include <errno.h>

/**
 * Intel Performance Primitives (reference):
 *
 * Structures and Enumerators:
 * https://software.intel.com/en-us/node/503715
 */

#if defined _WIN32 || defined __CYGWIN__
    #ifdef XPP_EXPORTS
        #ifdef __GNUC__
            #define XPP_EXPORT __attribute__((dllexport))
        #else
            #define XPP_EXPORT __declspec(dllexport)
        #endif
    #else
        #ifdef __GNUC__
            #define XPP_EXPORT __attribute__((dllimport))
        #else
            #define XPP_EXPORT __declspec(dllimport)
        #endif
    #endif
#else
    #if __GNUC__ >= 4
        #define XPP_EXPORT   __attribute__ ((visibility("default")))
    #else
        #define XPP_EXPORT
    #endif
#endif

#if defined _WIN32
#define XPP_API __stdcall
#else
#define XPP_API
#endif

typedef struct {
    int32_t x;
    int32_t y;
} XppPoint;

typedef struct {
    int32_t w;
    int32_t h;
} XppSize;

typedef struct {
    int16_t left;
    int16_t top;
    int16_t right;
    int16_t bottom;
} XppRect;

typedef enum xpp_interpolation_mode {
    XppInterpolationNearest = 0,
    XppInterpolationLinear = 1,
    XppInterpolationBilinear = 2,
    XppInterpolationBox = 3
} XppInterpolationMode;

typedef int32_t XppStatus;

enum xpp_status {
    XppSuccess =  0,
    XppFailure = -1,
};

#define xpp_malloc(_size)               malloc(_size)
#define xpp_calloc(_num, _size)            calloc(_num, _size)
#define xpp_realloc(_ptr, _size)        realloc(_ptr, _size)
#define xpp_free(_ptr)                    free(_ptr)

#define xpp_memcpy(_dst, _src, _size)   memcpy(_dst, _src, _size)
#define xpp_memset(_ptr, _val, _size)   memset(_ptr, _val, _size)

#endif /* XPP_H */
