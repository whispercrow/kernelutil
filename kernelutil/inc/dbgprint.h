#pragma once
#include <ntddk.h>

#define _LogMsg(lvl, lvlname, frmt, ...) { \
	DbgPrintEx( \
		DPFLTR_IHVDRIVER_ID, \
		lvl, \
		"[" lvlname "]" "[irql:%d pid:%-6Iu tid:%-6Iu %s::%-4d] " frmt "\n", \
		KeGetCurrentIrql(), \
		PsGetCurrentProcessId(), \
		PsGetCurrentThreadId(), \
		__FILE__, \
		__LINE__, \
		__VA_ARGS__ \
	); \
}

#define DbgError(frmt,   ...) _LogMsg(DPFLTR_ERROR_LEVEL,   "erro",   frmt, __VA_ARGS__)
#define DbgWarning(frmt, ...) _LogMsg(DPFLTR_WARNING_LEVEL, "warn", frmt, __VA_ARGS__)
#define DbgInfo(frmt,    ...) _LogMsg(DPFLTR_INFO_LEVEL,    "info",    frmt, __VA_ARGS__)
#define DbgTrace(frmt,   ...) _LogMsg(DPFLTR_TRACE_LEVEL,   "trac",   frmt, __VA_ARGS__)
#define DbgLog(frmt,   ...) _LogMsg(DPFLTR_ERROR_LEVEL,   "****",   frmt, __VA_ARGS__)
