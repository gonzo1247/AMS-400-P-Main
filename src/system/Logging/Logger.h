/*
Copyright 2023 - 2024 by Severin

*/
#pragma once

#include "LoggerDefines.h"
#include "Settings.h"

#include <QDate>
#include <QTime>

#include <fstream>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <codecvt>
#include <string>
#include <format>
#include <mutex>
#include <type_traits>
#include <cstdint>

#include "DatabaseTypes.h"

#ifdef _WIN32
#include <windows.h>
#include <objbase.h>   // StringFromGUID2
#include <rpc.h>
#endif

namespace fs = std::filesystem;


// ---------- UTF-8 Helper for wchar_t ----------
#ifdef _WIN32
inline std::string WideToUtf8(const wchar_t* ws, int len /*=-1 -> nullterm*/)
{
	if (!ws) return {};
	int n = WideCharToMultiByte(CP_UTF8, 0, ws, len, nullptr, 0, nullptr, nullptr);
	std::string out(n, '\0');
	if (n > 0) WideCharToMultiByte(CP_UTF8, 0, ws, len, out.data(), n, nullptr, nullptr);
	return out;
}
#endif

// Basic structure for the conversion
template <typename T>
struct ConvertIfQType
{
	static std::string convert(const T& v)
	{
		if constexpr (std::is_same_v<T, std::string>)
		{
			return v;
		}
		else if constexpr (std::is_arithmetic_v<T>)
		{
			// int, long, DWORD, DWORD64, double, ...
			return std::to_string(v);
		}
		else
		{
			// Try stream insert. If the type does not support this,
			// the specializations below provide the correct conversion.
			std::ostringstream oss;
			oss << v;
			return oss.str();
		}
	}
};

// ---------- C-String & Arrays ----------
template <size_t N>
struct ConvertIfQType<char[N]> {
	static std::string convert(const char(&s)[N]) { return std::string(s, (N && s[N - 1] == '\0') ? N - 1 : N); }
};
template <size_t N>
struct ConvertIfQType<const char[N]> {
	static std::string convert(const char(&s)[N]) { return std::string(s, (N && s[N - 1] == '\0') ? N - 1 : N); }
};
template <>
struct ConvertIfQType<char*> { static std::string convert(const char* s) { return s ? std::string(s) : std::string(); } };
template <>
struct ConvertIfQType<const char*> { static std::string convert(const char* s) { return s ? std::string(s) : std::string(); } };

#ifdef _WIN32
template <size_t N>
struct ConvertIfQType<wchar_t[N]> {
	static std::string convert(const wchar_t(&s)[N]) { return WideToUtf8(s, (N && s[N - 1] == L'\0') ? -1 : N); }
};
template <size_t N>
struct ConvertIfQType<const wchar_t[N]> {
	static std::string convert(const wchar_t(&s)[N]) { return WideToUtf8(s, (N && s[N - 1] == L'\0') ? -1 : N); }
};
template <> struct ConvertIfQType<wchar_t*> {
	static std::string convert(const wchar_t* s) { return WideToUtf8(s, -1); }
};
template <> struct ConvertIfQType<const wchar_t*> {
	static std::string convert(const wchar_t* s) { return WideToUtf8(s, -1); }
};
#endif

// ---------- Pointer & Handles ----------
template <typename P>
struct ConvertIfQType<P*>
{
	static std::string convert(P* p)
	{
		std::ostringstream oss;
		oss << "0x" << std::hex << reinterpret_cast<std::uintptr_t>(p);
		return oss.str();
	}
};

#ifdef _WIN32
template <> struct ConvertIfQType<HANDLE> {
	static std::string convert(HANDLE h) {
		std::ostringstream oss;
		oss << "0x" << std::hex << reinterpret_cast<std::uintptr_t>(h);
		return oss.str();
	}
};
#endif

// ---------- Qt ----------
template <> struct ConvertIfQType<QString> {
	static std::string convert(const QString& v) { return v.toStdString(); }
};
template <> struct ConvertIfQType<QByteArray> {
	static std::string convert(const QByteArray& v) { return v.toStdString(); }
};
template <> struct ConvertIfQType<QDate> {
	static std::string convert(const QDate& v) { return v.toString(Qt::ISODate).toStdString(); }
};
template <> struct ConvertIfQType<QTime> {
	static std::string convert(const QTime& v) { return v.toString(Qt::ISODate).toStdString(); }
};

// ---------- MySQL/Maria ----------
template <> struct ConvertIfQType<sql::SQLString> {
	static std::string convert(const sql::SQLString& s) {
#if defined(DB_BACKEND_MYSQL)
		return s.asStdString();
#else
		return std::string(s.c_str(), s.length());
#endif
	}
};

template <>
struct ConvertIfQType<sql::SQLException>
{
	static std::string convert(const sql::SQLException& e)
	{
		return std::string(e.what());
	}
};

template <>
struct ConvertIfQType<database::StatementName>
{
    static std::string convert(const database::StatementName& v)
    {
        switch (v)
        {
            case database::StatementName::NONE:
                return "NONE";
            default:
                return "StatementName(" + std::to_string(static_cast<std::uint32_t>(v)) + ")";
        }
    }
};

// ---------- Windows Basis-Typen ----------
#ifdef _WIN32
// DWORD, WORD, BYTE etc. are arithmetic and already fall into the basic template.
// For clarity here explicitly for DWORD64:
template <>
struct ConvertIfQType<DWORD64>
{
	static std::string convert(const DWORD64& v)
	{
		return std::to_string(static_cast<unsigned long long>(v));
	}
};

// LARGE_INTEGER / ULARGE_INTEGER
template <>
struct ConvertIfQType<LARGE_INTEGER>
{
	static std::string convert(const LARGE_INTEGER& li)
	{
		return std::to_string(static_cast<long long>(li.QuadPart));
	}
};
template <>
struct ConvertIfQType<ULARGE_INTEGER>
{
	static std::string convert(const ULARGE_INTEGER& li)
	{
		return std::to_string(static_cast<unsigned long long>(li.QuadPart));
	}
};

// FILETIME (100ns Ticks since 1601 UTC)
template <>
struct ConvertIfQType<FILETIME>
{
	static std::string convert(const FILETIME& ft)
	{
		ULARGE_INTEGER u{};
		u.LowPart = ft.dwLowDateTime;
		u.HighPart = ft.dwHighDateTime;
		std::ostringstream oss;
		oss << "FILETIME(100ns UTC)=" << static_cast<unsigned long long>(u.QuadPart);
		return oss.str();
	}
};

// SYSTEMTIME (lesbar)
template <>
struct ConvertIfQType<SYSTEMTIME>
{
	static std::string convert(const SYSTEMTIME& st)
	{
		char buf[64];
		std::snprintf(buf, sizeof(buf), "%04u-%02u-%02u %02u:%02u:%02u.%03u",
			st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
		return std::string(buf);
	}
};

// HRESULT (hex)
template <>
struct ConvertIfQType<HRESULT>
{
	static std::string convert(const HRESULT& hr)
	{
		char buf[16];
		std::snprintf(buf, sizeof(buf), "0x%08lX", static_cast<unsigned long>(hr));
		return std::string(buf);
	}
};

// GUID {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
template <>
struct ConvertIfQType<GUID>
{
	static std::string convert(const GUID& g)
	{
		char buf[40];
		std::snprintf(buf, sizeof(buf),
			"{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}",
			static_cast<unsigned long>(g.Data1),
			g.Data2, g.Data3,
			g.Data4[0], g.Data4[1], g.Data4[2], g.Data4[3],
			g.Data4[4], g.Data4[5], g.Data4[6], g.Data4[7]);
		return std::string(buf);
	}
};
#endif

// ---------- std::chrono duration nice ----------
template <class Rep, class Period>
struct ConvertIfQType<std::chrono::duration<Rep, Period>>
{
	static std::string convert(const std::chrono::duration<Rep, Period>& d)
	{
		using Dbl = std::chrono::duration<double, Period>;
		std::ostringstream oss;
		oss << std::chrono::duration_cast<Dbl>(d).count();
		return oss.str();
	}
};

// ---------- Threads, Error codes ----------
template <>
struct ConvertIfQType<std::thread::id>
{
	static std::string convert(const std::thread::id& id)
	{
		std::ostringstream oss; oss << id; return oss.str();
	}
};
template <>
struct ConvertIfQType<std::error_code>
{
	static std::string convert(const std::error_code& ec)
	{
		std::ostringstream oss; oss << ec.value() << ':' << ec.message(); return oss.str();
	}
};

class Logger
{
public:
	Logger();
	Logger(LoggerTypes types);
	~Logger();

	Logger(Logger const&) = delete;
	Logger(Logger&&) = delete;
	Logger& operator=(Logger const&) = delete;
	Logger& operator=(Logger&&) = delete;

	static Logger* instance();
	static void Init();

	void OpenOrCreateLogFile();

	void WriteLogFile(LoggerTypes type, LogLevel level, const std::string& formattedMessage);
	void WriteLogFile(LoggerTypes type, std::string_view format_str); // For backward compatibility
	void CloseLogFile();
	std::string ensurePath(const std::string& desiredPath) const;
	static bool IsLogLevelEnabled(LogLevel level);

	template <typename... Args>
	std::enable_if_t<(sizeof...(Args) > 0), void> OutMessage(LoggerTypes type, LogLevel level, std::string_view format_str, Args&&... args)
	{
		try
		{
			// Decay args so specializations (e.g. sql::SQLString) match
			auto convertedArgs = std::tuple{
				ConvertIfQType<std::remove_cvref_t<Args>>::convert(args)... };

			auto formattedMessage = std::vformat(format_str, std::apply([](const auto&... unpackedArgs) {
				return std::make_format_args(unpackedArgs...);
				}, convertedArgs));

			WriteLogFile(type, level, formattedMessage);
		}
		catch (const std::exception& e) {
			std::cerr << "Logging failed: " << e.what() << '\n';
		}
	}

	void OutMessage(LoggerTypes type, LogLevel level, std::string_view format_str)
	{
		WriteLogFile(type, level, std::string(format_str));
	}

private:
	void CloseAllLogFiles();
	static std::tm GetCurrentLocalTime();
	std::string GetLogLevelString(LogLevel level);

	std::ofstream _logFile;
	std::unordered_map<LoggerTypes, std::ofstream> _logFiles;
	LoggerTypes _loggerTypes;
	std::mutex _mutex;

	void OutMessageImpl(LoggerTypes level, std::string message);
};
