#include "inc/util_kernel.h"

void Sleep(_In_ LONG millisecond)
{
	LARGE_INTEGER Interval = {0};
	Interval.QuadPart = -1 * 10 * 1000 * millisecond;
	KeDelayExecutionThread(KernelMode, FALSE, &Interval);
}

UNICODE_STRING AllocUnicodeString(_In_ USHORT size, _In_ ULONG Tag, _In_opt_ POOL_TYPE PoolType)
{
	UNICODE_STRING usAlloc = { 0 };

	if (UNICODE_STRING_MAX_BYTES < size)
	{
		return usAlloc;
	}

	usAlloc.Buffer = (PWCHAR)ExAllocatePoolWithTag(PoolType, size, Tag);
	if (NULL != usAlloc.Buffer)
	{
		RtlZeroMemory(usAlloc.Buffer, size);
		usAlloc.Length = 0;
		usAlloc.MaximumLength = size;
	}

	return usAlloc;
}

UNICODE_STRING AllocCopyUnicodeString(_In_ PUNICODE_STRING pusString, _In_ ULONG  Tag, _In_opt_ POOL_TYPE PoolType)
{
	UNICODE_STRING usAlloc = { 0 };
	usAlloc.Buffer = (PWCHAR)ExAllocatePoolWithTag(PoolType, pusString->Length, Tag);
	if (NULL != usAlloc.Buffer)
	{
		RtlCopyMemory(usAlloc.Buffer, pusString->Buffer, pusString->Length);
		usAlloc.Length = pusString->Length;
		usAlloc.MaximumLength = usAlloc.Length;
	}

	return usAlloc;
}

VOID FreeUnicodeString(_In_ PUNICODE_STRING pusString)
{
	if (NULL == pusString)
	{
		return;
	}

	if (NULL != pusString->Buffer)
	{
		ExFreePool(pusString->Buffer);
	}

	pusString->Buffer = NULL;
	pusString->Length = 0;
	pusString->MaximumLength = 0;

	return;
}