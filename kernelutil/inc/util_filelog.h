#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <ntddk.h>
#ifdef __cplusplus
}
#endif

#include "cppruntime.h"

#pragma warning(disable:4200)

constexpr auto LOG_LIST_MAX_COUNT = 200000;
constexpr auto LOG_STRUCT_MAX_LENGTH = 2048;
constexpr auto LOG_FILE_MAX_SIZE = (20 * 1024 * 1024);

constexpr auto STRING_LOG_DEBGU = "dbug";
constexpr auto STRING_LOG_INFO = "info";
constexpr auto STRING_LOG_WARN = "warn";
constexpr auto STRING_LOG_ERROR = "erro";
constexpr auto STRING_LOG_LOG = "****";


enum LOG_LEVEL { LOG_LEVEL_DEBUG = 1, LOG_LEVEL_INFO, LOG_LEVEL_WARN, LOG_LEVEL_ERROR, LOG_LEVEL_LOG };

enum WORK_STATUS { LOG_WORK_WAITING, LOG_WORK_DEALING, LOG_WORK_STOPING, LOG_WORK_QUITED };

typedef struct
{
	LIST_ENTRY ListEntry;
	LOG_LEVEL LogLevel;
	LARGE_INTEGER LocalTime;
	CHAR Buffer[0];
} ST_LOG_ENTRY, *PST_LOG_ENTRY;


class KFileLog
{
public:
	KFileLog(_In_ POOL_TYPE Pool = NonPagedPool);
	KFileLog(const KFileLog&) = delete;
	KFileLog& operator=(const KFileLog&) = delete;
	virtual ~KFileLog();

	NTSTATUS Init(_In_ PUNICODE_STRING puniFilePath);

	void log(_In_ LOG_LEVEL level, _In_ char* format, _In_ ...);
	void SetLevelFilter(_In_ LOG_LEVEL level);

protected:
	void PushLogStructToList(_In_ PST_LOG_ENTRY pLogStruct);
	PST_LOG_ENTRY PopLogStructFromList();

	char* GenerateWriteData(_In_ PST_LOG_ENTRY pLogStruct);

	NTSTATUS ClearLogFile();
	NTSTATUS WriteLogFile(_In_ char* szWriteData);


	static void ThreadLogRoutine(_In_ PVOID StartContext);

private:
	BOOLEAN m_bInit = FALSE;
	LOG_LEVEL m_LevelFilter = LOG_LEVEL_DEBUG;
	WORK_STATUS m_WorkStatus = LOG_WORK_WAITING;
	POOL_TYPE m_Pool = NonPagedPool;

	HANDLE m_HandleFile = nullptr;
	ULONGLONG m_LogFileSize = 0;

	KSPIN_LOCK m_ListLock = {0};
	LIST_ENTRY m_LogList = {nullptr};
	ULONGLONG m_MemLogCount = 0;

	PETHREAD m_ThreadObj = nullptr;
	KEVENT m_ShutdownEvent = {0};
	KEVENT m_LogInsertEvent = {0};
};
