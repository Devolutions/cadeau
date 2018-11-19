#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

static unsigned int simd_type = ~0;

#if !defined(__aarch64__) && !defined(__APPLE__) && !defined(__ARM_NEON__) && (defined(__linux__) || defined(ANDROID) || defined(__ANDROID__))
#define NEED_CPU_FEATURE_DETECTION
#endif

#ifdef NEED_CPU_FEATURE_DETECTION

/*
 * NOTE: The ARM Android/Linux CPU feature detection code is from
 * libjpeg-turbo and is provided under the following license:
 *
 * Copyright (C) 2011, Nokia Corporation and/or its subsidiary(-ies).
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#define SOMEWHAT_SANE_PROC_CPUINFO_SIZE_LIMIT (1024 * 1024)

static int check_feature(char *buffer, char *feature)
{
	char *p;
	if (*feature == 0)
		return 0;
	if (strncmp(buffer, "Features", 8) != 0)
		return 0;
	buffer += 8;
	while (isspace(*buffer))
		buffer++;

	/* Check if 'feature' is present in the buffer as a separate word */
	while ((p = strstr(buffer, feature))) {
		if (p > buffer && !isspace(*(p - 1))) {
			buffer++;
			continue;
		}
		p += strlen(feature);
		if (*p != 0 && !isspace(*p)) {
			buffer++;
			continue;
		}
		return 1;
	}
	return 0;
}

static int parse_proc_cpuinfo(int bufsize)
{
	char *buffer = (char *)malloc(bufsize);
	FILE *fd;

	if (!buffer)
		return 0;

	fd = fopen("/proc/cpuinfo", "r");
	if (fd) {
		while (fgets(buffer, bufsize, fd)) {
			if (!strchr(buffer, '\n') && !feof(fd)) {
				/* "impossible" happened - insufficient size of the buffer! */
				fclose(fd);
				free(buffer);
				return 0;
			}
			if (check_feature(buffer, "neon"))
				simd_type = SIMD_NEON;
		}
		fclose(fd);
	}
	free(buffer);
	return 1;
}

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
	char *env = NULL;
#ifdef NEED_CPU_FEATURE_DETECTION
	int bufsize = 1024;  /* an initial guess for the line buffer size limit */
#endif

	if (simd_type != ~0U)
		return simd_type;

#if defined(__aarch64__) || defined(__APPLE__) || defined(__ARM_NEON__)
	/* NEON is always available with ARMv8, and any iPhones that lacked NEON
	   support (specifically, the 3G and earlier) are long obsolete. */

	simd_type = SIMD_NEON;

#elif defined(__linux__) || defined(ANDROID) || defined(__ANDROID__)

	while (!parse_proc_cpuinfo(bufsize)) {
		bufsize *= 2;
		if (bufsize > SOMEWHAT_SANE_PROC_CPUINFO_SIZE_LIMIT)
			break;
	}

#endif

	env = getenv("SIMD_FORCENONE");
	if ((env != NULL) && (strcmp(env, "1") == 0))
		simd_type = SIMD_NONE;

	return simd_type;
}
