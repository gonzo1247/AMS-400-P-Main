#include "CrashHandler.h"

#include <DbgHelp.h>
#include <QString>
#include <QDateTime>
#include <QDir>
#include <fstream>
#include <sstream>

#pragma comment(lib, "Dbghelp.lib")

void CrashHandler::WriteDump(EXCEPTION_POINTERS* exceptionPointers)
{
	// Ensure dump directory
	const QString dumpDir = "C:/ProgramData/Lager-und-Bestellverwaltung/exception";
	QDir().mkpath(dumpDir);

	// Create timestamp
	SYSTEMTIME localTime;
	GetLocalTime(&localTime);

	std::wstringstream fileNameStream;
	fileNameStream << L"crash_dump_ims_"
		<< localTime.wYear << L"-"
		<< localTime.wMonth << L"-"
		<< localTime.wDay << L"_"
		<< localTime.wHour << L"-"
		<< localTime.wMinute << L"-"
		<< localTime.wSecond << L".dmp";

	const std::wstring fileName = dumpDir.toStdWString() + L"/" + fileNameStream.str();

	// Open dump file
	HANDLE hFile = CreateFileW(fileName.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if ((hFile == nullptr) || (hFile == INVALID_HANDLE_VALUE))
		return;

	// MiniDump-Infos
	MINIDUMP_EXCEPTION_INFORMATION mdei;
	mdei.ThreadId = GetCurrentThreadId();
	mdei.ExceptionPointers = exceptionPointers;
	mdei.ClientPointers = FALSE;

	// Dump type with extended info
	const auto dumpType = static_cast<MINIDUMP_TYPE>(
		MiniDumpWithFullMemory |
		MiniDumpWithDataSegs |
		MiniDumpWithHandleData |
		MiniDumpWithUnloadedModules |
		MiniDumpWithThreadInfo |
		MiniDumpWithProcessThreadData
		);

	// Write dump
	MiniDumpWriteDump(
		GetCurrentProcess(),
		GetCurrentProcessId(),
		hFile,
		dumpType,
		(exceptionPointers != nullptr) ? &mdei : nullptr,
		nullptr,
		nullptr
	);

	CloseHandle(hFile);
}
