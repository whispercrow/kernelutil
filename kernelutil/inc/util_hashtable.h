#pragma once
#include <ntddk.h>
#include "cppruntime.h"

#pragma warning(disable:4200)

class KernelHashTable
{
public:
	KernelHashTable(_In_ POOL_TYPE PoolType = NonPagedPool);
	virtual ~KernelHashTable();

	NTSTATUS Insert(_In_ ULONG HashCode, _In_ PVOID Context, _In_ ULONG ContextSize);
	NTSTATUS GetNeedLock(_In_ ULONG HashCode, _Out_ PVOID* pContext, _Out_opt_ ULONG* pContextSize = nullptr);
	//please put GetNeedLock and pContext between Lock and UnLock
	BOOLEAN Remove(_In_ ULONG HashCode);
	BOOLEAN Exist(_In_ ULONG HashCode);
	ULONG Count();
	VOID Clear();

	VOID Lock();
	VOID UnLock();

protected:
	static RTL_GENERIC_COMPARE_RESULTS RoutineCompare(_In_ struct _RTL_GENERIC_TABLE* Table, _In_ PVOID FirstStruct,
	                                                  _In_ PVOID SecondStruct);
	static PVOID RoutineAllocate(_In_ struct _RTL_GENERIC_TABLE* Table, _In_ CLONG ByteSize);
	static VOID RoutineFree(_In_ struct _RTL_GENERIC_TABLE* Table, _In_ PVOID Buffer);

private:
	typedef struct
	{
		ULONG HashCode;
		ULONG nContextSize;
		CHAR pUserContext[0];
	} HASH_NODE, *PHASH_NODE;

	RTL_GENERIC_TABLE m_Table = {nullptr};
	FAST_MUTEX m_Lock = {0};
	POOL_TYPE m_Pool = PagedPool;
};
