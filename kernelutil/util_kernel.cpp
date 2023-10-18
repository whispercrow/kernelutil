#include "inc/util_kernel.h"

void Sleep(_In_ LONG millisecond)
{
	LARGE_INTEGER Interval = {0};
	Interval.QuadPart = -1 * 10 * 1000 * millisecond;
	KeDelayExecutionThread(KernelMode, FALSE, &Interval);
}
