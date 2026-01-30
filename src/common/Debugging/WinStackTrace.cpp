#include <DbgHelp.h>
#include <Windows.h>

#include <mutex>
#include <string>
#include <vector>

#include "WinStackTrace.h"

#include "Logger.h"
#include "LoggerDefines.h"

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

    std::string Narrow(const wchar_t* ws)
    {
        if (!ws)
            return {};

        int len = WideCharToMultiByte(CP_UTF8, 0, ws, -1, nullptr, 0, nullptr, nullptr);
        if (len <= 1)
            return {};

        std::string out(static_cast<std::size_t>(len - 1), '\0');
        WideCharToMultiByte(CP_UTF8, 0, ws, -1, out.data(), len, nullptr, nullptr);
        return out;
    }
}  // namespace

void LogCurrentStackTrace(std::size_t skipFrames, std::size_t maxFrames)
{
    InitSymbols();

    constexpr std::size_t MaxCap = 128;
    const std::size_t cap = (maxFrames > MaxCap) ? MaxCap : maxFrames;

    void* stack[MaxCap] = {};
    const USHORT captured =
        CaptureStackBackTrace(static_cast<DWORD>(skipFrames + 1), static_cast<DWORD>(cap), stack, nullptr);

    HANDLE proc = GetCurrentProcess();

    for (USHORT i = 0; i < captured; ++i)
    {
        const DWORD64 addr = reinterpret_cast<DWORD64>(stack[i]);

        // Symbol
        char symBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {};
        auto* sym = reinterpret_cast<SYMBOL_INFO*>(symBuffer);
        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
        sym->MaxNameLen = MAX_SYM_NAME;

        std::string symbolName = "<unknown>";
        DWORD64 displacementSym = 0;

        if (SymFromAddr(proc, addr, &displacementSym, sym))
            symbolName = sym->Name;

        // Module
        IMAGEHLP_MODULE64 modInfo = {};
        modInfo.SizeOfStruct = sizeof(modInfo);

        std::string moduleName = "<unknown-module>";
        DWORD64 moduleBase = 0;

        if (SymGetModuleInfo64(proc, addr, &modInfo))
        {
            moduleName = modInfo.ModuleName ? modInfo.ModuleName : moduleName;
            moduleBase = modInfo.BaseOfImage;
        }

        // Line info (requires PDBs; may be empty in release)
        IMAGEHLP_LINE64 lineInfo = {};
        lineInfo.SizeOfStruct = sizeof(lineInfo);

        DWORD displacementLine = 0;
        bool hasLine = SymGetLineFromAddr64(proc, addr, &displacementLine, &lineInfo) != FALSE;

        if (hasLine)
        {
            LOG_MISC("  at {}!{} +0x{:X} ({}:{}) [0x{:X}]", moduleName, symbolName,
                     static_cast<unsigned long long>(displacementSym),
                     lineInfo.FileName ? lineInfo.FileName : "<unknown-file>", lineInfo.LineNumber,
                     static_cast<unsigned long long>(addr));
        }
        else if (moduleBase != 0)
        {
            const auto rva = addr - moduleBase;
            LOG_MISC("  at {}!{} +0x{:X} (rva=0x{:X}) [0x{:X}]", moduleName, symbolName,
                     static_cast<unsigned long long>(displacementSym), static_cast<unsigned long long>(rva),
                     static_cast<unsigned long long>(addr));
        }
        else
        {
            LOG_MISC("  at {} +0x{:X} [0x{:X}]", symbolName, static_cast<unsigned long long>(displacementSym),
                     static_cast<unsigned long long>(addr));
        }
    }
}
