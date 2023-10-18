#pragma once
#include <ntddk.h>

void Sleep(_In_ LONG millisecond);

UNICODE_STRING AllocUnicodeString(_In_ USHORT size, _In_ ULONG Tag, _In_opt_ POOL_TYPE PoolType = NonPagedPool);

UNICODE_STRING AllocCopyUnicodeString(_In_ PUNICODE_STRING pusString, _In_ ULONG  Tag, _In_opt_ POOL_TYPE PoolType = NonPagedPool);

VOID FreeUnicodeString(_In_ PUNICODE_STRING pusString);