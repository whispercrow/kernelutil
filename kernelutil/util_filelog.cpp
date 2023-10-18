#include "inc/util_filelog.h"
#include <ntstrsafe.h>

KFileLog::KFileLog(_In_ POOL_TYPE Pool)
{
	KeInitializeEvent(&m_ShutdownEvent, NotificationEvent, FALSE);
	KeInitializeEvent(&m_LogInsertEvent, SynchronizationEvent, FALSE);
	InitializeListHead(&m_LogList);
	KeInitializeSpinLock(&m_ListLock);
	m_Pool = Pool;
}

KFileLog::~KFileLog()
{
	KeSetEvent(&m_ShutdownEvent, NULL, FALSE);
	if (nullptr != m_ThreadObj)
	{
		KeWaitForSingleObject(m_ThreadObj, Executive, KernelMode, FALSE, nullptr);
		ObDereferenceObject(m_ThreadObj);
		m_WorkStatus = LOG_WORK_QUITED;
	}


	ZwClose(m_HandleFile);


	while (auto pItem = PopLogStructFromList())
	{
		delete[] pItem;
	}
}

NTSTATUS KFileLog::Init(_In_ PUNICODE_STRING puniFilePath)
{
	if (m_bInit)
	{
		return STATUS_ALREADY_INITIALIZED;
	}

	NTSTATUS status = STATUS_SUCCESS;
	IO_STATUS_BLOCK IoStatus = {0};
	OBJECT_ATTRIBUTES ObjectAttributes = {0};
	FILE_STANDARD_INFORMATION FileStdInfo = {0};
	HANDLE hThread = nullptr;

	InitializeObjectAttributes(&ObjectAttributes, puniFilePath, ((OBJ_CASE_INSENSITIVE) | (OBJ_KERNEL_HANDLE)), NULL,
	                           NULL);
	status = ZwCreateFile(
		&m_HandleFile,
		FILE_APPEND_DATA | SYNCHRONIZE,
		&ObjectAttributes,
		&IoStatus,
		nullptr,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ,
		FILE_OPEN_IF,
		FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
		nullptr,
		0);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	status = ZwQueryInformationFile(m_HandleFile, &IoStatus, &FileStdInfo, sizeof(FileStdInfo),
	                                FileStandardInformation);
	if (!NT_SUCCESS(status))
	{
		ZwClose(m_HandleFile);
		m_HandleFile = nullptr;
		return status;
	}

	m_LogFileSize = FileStdInfo.EndOfFile.QuadPart;

	status = PsCreateSystemThread(&hThread, GENERIC_ALL, nullptr, nullptr, nullptr, ThreadLogRoutine, this);
	if (!NT_SUCCESS(status))
	{
		ZwClose(m_HandleFile);
		m_HandleFile = nullptr;
		m_LogFileSize = 0;
		return status;
	}

	ObReferenceObjectByHandle(hThread, THREAD_ALL_ACCESS, nullptr, KernelMode, (PVOID*)&m_ThreadObj, nullptr);
	ZwClose(hThread);

	m_bInit = TRUE;
	return STATUS_SUCCESS;
}

void KFileLog::log(_In_ LOG_LEVEL level, _In_ char* format, _In_ ...)
{
	if (!m_bInit)
	{
		return;
	}

	/*if (KeGetCurrentIrql() > APC_LEVEL)
	{
		return;
	}*/

	if (m_WorkStatus == LOG_WORK_STOPING || m_WorkStatus == LOG_WORK_QUITED)
	{
		return;
	}

	if (level < m_LevelFilter)
	{
		return;
	}

	LARGE_INTEGER LocalTime = {0};
	KeQuerySystemTime(&LocalTime);
	ExSystemTimeToLocalTime(&LocalTime, &LocalTime);

	auto pLogStruct = (PST_LOG_ENTRY)new(m_Pool) char[LOG_STRUCT_MAX_LENGTH];
	if (nullptr == pLogStruct)
	{
		return;
	}

	memset(pLogStruct, 0, LOG_STRUCT_MAX_LENGTH);
	pLogStruct->LogLevel = level;
	pLogStruct->LocalTime = LocalTime;

	va_list args;
	va_start(args, format);
	NTSTATUS status = RtlStringCchVPrintfA(pLogStruct->Buffer,
	                                       LOG_STRUCT_MAX_LENGTH - (pLogStruct->Buffer - (char*)pLogStruct) - 1, format,
	                                       args);
	va_end(args);

	if (STATUS_INVALID_PARAMETER == status)
	{
		delete[] pLogStruct;
		return;
	}


	PushLogStructToList(pLogStruct);
}

void KFileLog::SetLevelFilter(_In_ LOG_LEVEL level)
{
	m_LevelFilter = level;
}

NTSTATUS KFileLog::ClearLogFile()
{
	IO_STATUS_BLOCK IoStatus = {0};
	FILE_END_OF_FILE_INFORMATION FileEndInfo = {0};

	NTSTATUS status = ZwSetInformationFile(m_HandleFile, &IoStatus, &FileEndInfo, sizeof(FileEndInfo),
	                                       FileEndOfFileInformation);
	if (NT_SUCCESS(status))
	{
		m_LogFileSize = 0;
	}

	return status;
}

NTSTATUS KFileLog::WriteLogFile(_In_ char* szWriteData)
{
	IO_STATUS_BLOCK IoStatus = {0};

	NTSTATUS status = ZwWriteFile(m_HandleFile, nullptr, nullptr, nullptr, &IoStatus, szWriteData,
	                              static_cast<ULONG>(strlen(szWriteData)), nullptr, nullptr);
	if (NT_SUCCESS(status))
	{
		m_LogFileSize += IoStatus.Information;
	}

	return status;
}

void KFileLog::PushLogStructToList(_In_ PST_LOG_ENTRY pLogStruct)
{
	PLIST_ENTRY pOldEntry = nullptr;
	KIRQL oldIrql = DISPATCH_LEVEL;

	KeAcquireSpinLock(&m_ListLock, &oldIrql);
	if (LOG_LIST_MAX_COUNT < m_MemLogCount)
	{
		pOldEntry = RemoveHeadList(&m_LogList);
		m_MemLogCount--;
	}
	InsertTailList(&m_LogList, &pLogStruct->ListEntry);
	m_MemLogCount++;

	KeReleaseSpinLock(&m_ListLock, oldIrql);

	KeSetEvent(&m_LogInsertEvent, 0, FALSE);

	if (nullptr != pOldEntry)
	{
		auto pOldItem = CONTAINING_RECORD(pOldEntry, ST_LOG_ENTRY, ListEntry);
		delete[] pOldItem;
	}
}

PST_LOG_ENTRY KFileLog::PopLogStructFromList()
{
	PST_LOG_ENTRY pPickItem = nullptr;
	KIRQL oldIrql = DISPATCH_LEVEL;

	KeAcquireSpinLock(&m_ListLock, &oldIrql);
	PLIST_ENTRY pPickEntry = RemoveHeadList(&m_LogList);
	if (pPickEntry != &m_LogList)
	{
		pPickItem = CONTAINING_RECORD(pPickEntry, ST_LOG_ENTRY, ListEntry);
		m_MemLogCount--;
	}
	KeReleaseSpinLock(&m_ListLock, oldIrql);


	return pPickItem;
}

char* KFileLog::GenerateWriteData(_In_ PST_LOG_ENTRY pLogStruct)
{
	size_t nBufferLen = strlen(pLogStruct->Buffer) + 64;
	auto WriteBuffer = new(m_Pool) char[nBufferLen];
	memset(WriteBuffer, 0, nBufferLen);

	const char* szLeveltag = nullptr;
	switch (pLogStruct->LogLevel)
	{
	case LOG_LEVEL_DEBUG:
		szLeveltag = STRING_LOG_DEBGU;
		break;
	case LOG_LEVEL_INFO:
		szLeveltag = STRING_LOG_INFO;
		break;
	case LOG_LEVEL_WARN:
		szLeveltag = STRING_LOG_WARN;
		break;
	case LOG_LEVEL_ERROR:
		szLeveltag = STRING_LOG_ERROR;
		break;
	case LOG_LEVEL_LOG:
	default:
		szLeveltag = STRING_LOG_LOG;
	}

	TIME_FIELDS TimeFiled = {0};
	RtlTimeToTimeFields(&pLogStruct->LocalTime, &TimeFiled);

	sprintf(WriteBuffer, "[%s][%04d-%02d-%02d %02d:%02d:%02d.%03d] %s\n", szLeveltag,
	        TimeFiled.Year,
	        TimeFiled.Month,
	        TimeFiled.Day,
	        TimeFiled.Hour,
	        TimeFiled.Minute,
	        TimeFiled.Second,
	        TimeFiled.Milliseconds,
	        pLogStruct->Buffer
	);

	return WriteBuffer;
}

void KFileLog::ThreadLogRoutine(_In_ PVOID StartContext)
{
	if (KeGetCurrentIrql() > APC_LEVEL)
	{
		PsTerminateSystemThread(-1);
		return;
	}

	auto pThis = static_cast<KFileLog*>(StartContext);
	PVOID WaitEventArray[] = {&pThis->m_ShutdownEvent, &pThis->m_LogInsertEvent};

	while (TRUE)
	{
		pThis->m_WorkStatus = LOG_WORK_WAITING;
		NTSTATUS WaitStatus = KeWaitForMultipleObjects(sizeof(WaitEventArray) / sizeof(*WaitEventArray), WaitEventArray,
		                                               WaitAny, Executive, KernelMode, FALSE, nullptr, nullptr);
		if (!NT_SUCCESS(WaitStatus))
		{
			break;
		}

		ULONG nSignalIndex = WaitStatus - STATUS_WAIT_0;

		if (0 == nSignalIndex)
		{
			pThis->m_WorkStatus = LOG_WORK_STOPING;
			break;
		}
		if (1 == nSignalIndex)
		{
			pThis->m_WorkStatus = LOG_WORK_DEALING;
			while (auto pItem = pThis->PopLogStructFromList())
			{
				if (pThis->m_LogFileSize > LOG_FILE_MAX_SIZE)
				{
					pThis->ClearLogFile();
				}

				auto WriteBuffer = pThis->GenerateWriteData(pItem);
				if (nullptr != WriteBuffer)
				{
					pThis->WriteLogFile(WriteBuffer);
					delete[] WriteBuffer;
				}

				delete[] pItem;
			}
		}
		else
		{
			pThis->m_WorkStatus = LOG_WORK_STOPING;
			break;
		}
	}

	PsTerminateSystemThread(0);
}
