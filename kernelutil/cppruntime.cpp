#include "inc/cppruntime.h"

#pragma warning(disable:4100)
#pragma warning(disable:4996)

void*__cdecl operator new(size_t size)
{
	void* p = ExAllocatePoolWithTag(NonPagedPool, size, DRIVER_TAG);
	if (nullptr != p)
	{
#if DBG
		memset(p, 0xcc, size);
#else
		memset(p, 0, size);
#endif
	}
	return p;
}


void*__cdecl operator new(size_t size, POOL_TYPE pool, ULONG tag)
{
	void* p = ExAllocatePoolWithTag(pool, size, tag);
	if (nullptr != p)
	{
#if DBG
		memset(p, 0xcc, size);
#else
		memset(p, 0, size);
#endif
	}
	return p;
}


void*__cdecl operator new[](size_t size)
{
	void* p = ExAllocatePoolWithTag(NonPagedPool, size, DRIVER_TAG);
	if (nullptr != p)
	{
#if DBG
		memset(p, 0xcc, size);
#else
		memset(p, 0, size);
#endif
	}
	return p;
}


void*__cdecl operator new[](size_t size, POOL_TYPE pool, ULONG tag)
{
	void* p = ExAllocatePoolWithTag(pool, size, tag);
	if (nullptr != p)
	{
#if DBG
		memset(p, 0xcc, size);
#else
		memset(p, 0, size);
#endif
	}
	return p;
}


void*__cdecl operator new(size_t Size, void* p)
{
	return p;
}


void __cdecl operator delete(void* p, size_t)
{
	ExFreePool(p);
}


void __cdecl operator delete[](void* p)
{
	ExFreePool(p);
}
