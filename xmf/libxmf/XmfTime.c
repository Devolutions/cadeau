
#include "XmfTime.h"

#include <winpr/sysinfo.h>

uint64_t XmfTime_Get()
{
	return GetTickCount64();
}
