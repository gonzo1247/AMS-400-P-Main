#include "CrashReport.h"

#include <DbgHelp.h>
#include <Windows.h>

#include <atomic>
#include <cstdio>
#include <mutex>
#include <string>

#pragma comment(lib, "Dbghelp.lib")

namespace
{
    std::once_flag g_SymInitOnce;

    void InitSymbols()
    {
        std::call_once(g_SymInitOnce,
                       []()
                       {
                           HANDLE proc = GetCurrentProcess();
                           SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_FAIL_CRITICAL_ERRORS);
                           SymInitialize(proc, nullptr, TRUE);
                       });
    }

    void EnsureDir(const wchar_t* dir) { CreateDirectoryW(dir, nullptr); }

    std::wstring BuildLogPath()
    {
        const wchar_t* dir = L"C:\\ProgramData\\AMS\\exception";
        EnsureDir(dir);

        SYSTEMTIME st;
        GetLocalTime(&st);

        wchar_t fileName[256] = {};
        _snwprintf_s(fileName, _TRUNCATE, L"%s\\critical_error_log_ams_%04u-%02u-%02u-%02u-%02u-%02u.log", dir, st.wYear, st.wMonth,
                     st.wDay, st.wHour, st.wMinute, st.wSecond);

        return fileName;
    }

    void WriteLine(HANDLE h, const std::string& s)
    {
        if (h == INVALID_HANDLE_VALUE)
            return;

        DWORD written = 0;
        WriteFile(h, s.data(), static_cast<DWORD>(s.size()), &written, nullptr);
    }

    std::string FormatTime()
    {
        SYSTEMTIME st;
        GetLocalTime(&st);

        char buf[64] = {};
        sprintf_s(buf, "%04u-%02u-%02u %02u:%02u:%02u", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

        return buf;
    }
}  // namespace

void WriteExceptionReportToFile(const char* kind, const char* message, std::size_t skipFrames, std::size_t maxFrames)
{
    InitSymbols();

    const std::wstring path = BuildLogPath();

    HANDLE h = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, nullptr);

    const DWORD tid = GetCurrentThreadId();

    std::string header;
    header.reserve(512);
    header += "==== Exception Report (";
    header += FormatTime();
    header += ") ====\n";
    header += "Kind: ";
    header += (kind ? kind : "unknown");
    header += "\n";
    header += "ThreadId: ";
    header += std::to_string(tid);
    header += "\n";
    header += "Message: ";
    header += (message ? message : "");
    header += "\n";
    header += "-- StackTrace --\n";

    WriteLine(h, header);
    OutputDebugStringA(header.c_str());

    constexpr std::size_t MaxCap = 128;
    const std::size_t cap = (maxFrames > MaxCap) ? MaxCap : maxFrames;

    void* stack[MaxCap] = {};
    const USHORT captured =
        CaptureStackBackTrace(static_cast<DWORD>(skipFrames + 1), static_cast<DWORD>(cap), stack, nullptr);

    HANDLE proc = GetCurrentProcess();

    for (USHORT i = 0; i < captured; ++i)
    {
        const DWORD64 addr = reinterpret_cast<DWORD64>(stack[i]);

        char symBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {};
        auto* sym = reinterpret_cast<SYMBOL_INFO*>(symBuffer);
        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
        sym->MaxNameLen = MAX_SYM_NAME;

        std::string symbolName = "<unknown>";
        DWORD64 dispSym = 0;

        if (SymFromAddr(proc, addr, &dispSym, sym))
            symbolName = sym->Name;

        IMAGEHLP_MODULE64 modInfo = {};
        modInfo.SizeOfStruct = sizeof(modInfo);

        std::string moduleName = "<unknown-module>";
        DWORD64 moduleBase = 0;

        if (SymGetModuleInfo64(proc, addr, &modInfo))
        {
            if (modInfo.ModuleName)
                moduleName = modInfo.ModuleName;
            moduleBase = modInfo.BaseOfImage;
        }

        IMAGEHLP_LINE64 lineInfo = {};
        lineInfo.SizeOfStruct = sizeof(lineInfo);

        DWORD dispLine = 0;
        const bool hasLine = SymGetLineFromAddr64(proc, addr, &dispLine, &lineInfo) != FALSE;

        std::string line;
        line.reserve(512);

        if (hasLine)
        {
            line += "  at ";
            line += moduleName;
            line += "!";
            line += symbolName;
            line += " +0x";
            line += std::to_string(static_cast<unsigned long long>(dispSym));
            line += " (";
            line += (lineInfo.FileName ? lineInfo.FileName : "<unknown-file>");
            line += ":";
            line += std::to_string(lineInfo.LineNumber);
            line += ") [0x";
        }
        else if (moduleBase != 0)
        {
            const auto rva = addr - moduleBase;

            line += "  at ";
            line += moduleName;
            line += "!";
            line += symbolName;
            line += " (rva=0x";
            char rvaBuf[32] = {};
            sprintf_s(rvaBuf, "%llX", static_cast<unsigned long long>(rva));
            line += rvaBuf;
            line += ") [0x";
        }
        else
        {
            line += "  at ";
            line += symbolName;
            line += " [0x";
        }

        char addrBuf[32] = {};
        sprintf_s(addrBuf, "%llX", static_cast<unsigned long long>(addr));
        line += addrBuf;
        line += "]\n";

        WriteLine(h, line);
        OutputDebugStringA(line.c_str());
    }

    WriteLine(h, "==== End ====\n\n");
    OutputDebugStringA("==== End ====\n\n");

    if (h != INVALID_HANDLE_VALUE)
        CloseHandle(h);
}
