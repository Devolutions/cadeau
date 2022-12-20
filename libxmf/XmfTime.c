
#include "XmfTime.h"

#ifndef _WIN32
#include <time.h>
#include <sys/time.h>
#endif

uint64_t XmfTime_GetTickCount()
{
    uint64_t ticks = 0;
#if defined(_WIN32)
    ticks = GetTickCount64();
#elif defined(__linux__)
    struct timespec ts;
    if (!clock_gettime(CLOCK_MONOTONIC_RAW, &ts))
        ticks = (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
#else
    struct timeval tv;
    if (!gettimeofday(&tv, NULL))
        ticks = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
#endif
    return ticks;
}

uint64_t XmfTimeSource_Get(XmfTimeSource* ts)
{
    if (!ts || !ts->func)
        return 0;

    return ts->func(ts->param);
}

uint64_t XmfTimeSource_System(void* param)
{
    return XmfTime_GetTickCount();
}

uint64_t XmfTimeSource_Manual(void* param)
{
    return *((uint64_t*) param);
}
