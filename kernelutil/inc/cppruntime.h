#pragma once
#include <ntddk.h>

#define DRIVER_TAG 'dsiw'

void* __cdecl operator new(size_t size);

void* __cdecl operator new(size_t size, POOL_TYPE pool, ULONG tag = DRIVER_TAG);

void* __cdecl operator new[](size_t size);

void*__cdecl operator new[](size_t size, POOL_TYPE pool, ULONG tag = DRIVER_TAG);

void*__cdecl operator new(size_t Size, void* p);

void __cdecl operator delete(void* p, size_t);

void __cdecl operator delete[](void* p);
