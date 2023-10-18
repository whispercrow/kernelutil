#include "inc/util_hashtable.h"

#pragma warning(disable:4100)

KernelHashTable::KernelHashTable(_In_ POOL_TYPE PoolType):
	m_Pool(PoolType)
{
	ExInitializeFastMutex(&m_Lock);
	RtlInitializeGenericTable(&m_Table, RoutineCompare, RoutineAllocate, RoutineFree, this);
}

KernelHashTable::~KernelHashTable()
{
	Clear();
}

NTSTATUS KernelHashTable::Insert(_In_ ULONG HashCode, _In_ PVOID Context, _In_ ULONG ContextSize)
{
	ULONG nInsertNodeSize = static_cast<ULONG>(sizeof(HASH_NODE)) + (Context ? ContextSize : 0);
	auto pInsertNode = (PHASH_NODE)new(m_Pool) char[nInsertNodeSize];
	pInsertNode->HashCode = HashCode;
	pInsertNode->nContextSize = Context ? ContextSize : 0;
	if (Context)
	{
		memcpy(pInsertNode->pUserContext, Context, ContextSize);
	}

	NTSTATUS status = STATUS_SUCCESS;
	BOOLEAN bNewInsert = FALSE;

	ExAcquireFastMutex(&m_Lock);
	if (!RtlInsertElementGenericTable(&m_Table, pInsertNode, nInsertNodeSize, &bNewInsert))
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
	}
	else
	{
		if (FALSE == bNewInsert)
		{
			status = STATUS_OBJECT_NAME_COLLISION;
		}
	}
	ExReleaseFastMutex(&m_Lock);

	delete[] pInsertNode;
	return status;
}


NTSTATUS KernelHashTable::GetNeedLock(_In_ ULONG HashCode, _Out_ PVOID* pContext, _Out_opt_ ULONG* pContextSize)
{
	HASH_NODE HashNode = {HashCode};
	NTSTATUS status = STATUS_SUCCESS;

	auto pNode = static_cast<PHASH_NODE>(RtlLookupElementGenericTable(&m_Table, &HashNode));
	if (nullptr == pNode)
	{
		status = STATUS_OBJECT_NAME_NOT_FOUND;
	}
	else
	{
		if (pNode->nContextSize != 0)
		{
			*pContext = pNode->pUserContext;
		}

		if (nullptr != pContextSize)
		{
			*pContextSize = pNode->nContextSize;
		}
	}

	return status;
}

BOOLEAN KernelHashTable::Remove(_In_ ULONG HashCode)
{
	HASH_NODE HashNode = {HashCode};
	BOOLEAN bRemove = FALSE;

	ExAcquireFastMutex(&m_Lock);
	if (RtlDeleteElementGenericTable(&m_Table, &HashNode))
	{
		bRemove = TRUE;
	}
	ExReleaseFastMutex(&m_Lock);

	return bRemove;
}

BOOLEAN KernelHashTable::Exist(_In_ ULONG HashCode)
{
	HASH_NODE HashNode = {HashCode};
	BOOLEAN bExist = FALSE;

	ExAcquireFastMutex(&m_Lock);
	if (RtlLookupElementGenericTable(&m_Table, &HashNode))
	{
		bExist = TRUE;
	}
	ExReleaseFastMutex(&m_Lock);

	return bExist;
}

ULONG KernelHashTable::Count()
{
	return RtlNumberGenericTableElements(&m_Table);
}

VOID KernelHashTable::Clear()
{
	ExAcquireFastMutex(&m_Lock);
	while (!RtlIsGenericTableEmpty(&m_Table))
	{
		auto pNode = static_cast<PHASH_NODE>(RtlGetElementGenericTable(
			&m_Table, RtlNumberGenericTableElements(&m_Table) - 1));
		RtlDeleteElementGenericTable(&m_Table, pNode);
	}

	ExReleaseFastMutex(&m_Lock);
}

VOID KernelHashTable::Lock()
{
	ExAcquireFastMutex(&m_Lock);
}

VOID KernelHashTable::UnLock()
{
	ExReleaseFastMutex(&m_Lock);
}

RTL_GENERIC_COMPARE_RESULTS KernelHashTable::RoutineCompare(_In_ struct _RTL_GENERIC_TABLE* Table,
                                                            _In_ PVOID FirstStruct, _In_ PVOID SecondStruct)
{
	auto pFirstNode = static_cast<PHASH_NODE>(FirstStruct);
	auto pSecondNode = static_cast<PHASH_NODE>(SecondStruct);

	if (pFirstNode->HashCode < pSecondNode->HashCode)
	{
		return GenericLessThan;
	}
	if (pFirstNode->HashCode > pSecondNode->HashCode)
	{
		return GenericGreaterThan;
	}

	return GenericEqual;
}

PVOID KernelHashTable::RoutineAllocate(_In_ struct _RTL_GENERIC_TABLE* Table, _In_ CLONG ByteSize)
{
	return new(static_cast<KernelHashTable*>(Table->TableContext)->m_Pool) char[ByteSize];
}

VOID KernelHashTable::RoutineFree(_In_ struct _RTL_GENERIC_TABLE* Table, _In_ PVOID Buffer)
{
	delete[] Buffer;
}
