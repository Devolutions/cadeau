#ifndef XMF_H
#define XMF_H

/**
 * XMF: eXtreme Media Foundation
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

#if defined _WIN32 || defined __CYGWIN__
	#ifdef XMF_EXPORTS
		#ifdef __GNUC__
			#define XMF_EXPORT __attribute__((dllexport))
		#else
			#define XMF_EXPORT __declspec(dllexport)
		#endif
	#else
		#ifdef __GNUC__
			#define XMF_EXPORT __attribute__((dllimport))
		#else
			#define XMF_EXPORT __declspec(dllimport)
		#endif
	#endif
#else
	#if __GNUC__ >= 4
		#define XMF_EXPORT   __attribute__ ((visibility("default")))
	#else
		#define XMF_EXPORT
	#endif
#endif

#if defined _WIN32
#define XMF_API __stdcall
#else
#define XMF_API
#endif

#endif /* XMF_H */
