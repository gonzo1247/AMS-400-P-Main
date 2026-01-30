#include "Diagnostics.h"

#include <iostream>
#include <QSysInfo>
#include <QString>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <psapi.h>
#include <QDir>

#include "CrashHandler.h"

namespace
{
	QString getModulePath(const wchar_t* moduleName)
	{
		HMODULE hMod = GetModuleHandleW(moduleName);
		if (!hMod)
			return "Not loaded";

		wchar_t path[MAX_PATH] = { 0 };
		if (GetModuleFileNameW(hMod, path, MAX_PATH))
			return QString::fromWCharArray(path);

		return "Error retrieving path";
	}

	QString getArchitecture()
	{
#if defined(Q_PROCESSOR_X86_64)
		return "x64";
#elif defined(Q_PROCESSOR_X86)
		return "x86";
#else
		return "Unknown";
#endif
	}

	QString exceptionCodeToString(DWORD code)
	{
		switch (code)
		{
			case EXCEPTION_ACCESS_VIOLATION: return "EXCEPTION_ACCESS_VIOLATION";
			case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
			case EXCEPTION_BREAKPOINT: return "EXCEPTION_BREAKPOINT";
			case EXCEPTION_DATATYPE_MISALIGNMENT: return "EXCEPTION_DATATYPE_MISALIGNMENT";
			case EXCEPTION_FLT_DENORMAL_OPERAND: return "EXCEPTION_FLT_DENORMAL_OPERAND";
			case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
			case EXCEPTION_ILLEGAL_INSTRUCTION: return "EXCEPTION_ILLEGAL_INSTRUCTION";
			case EXCEPTION_INT_DIVIDE_BY_ZERO: return "EXCEPTION_INT_DIVIDE_BY_ZERO";
			default: return QString("0x%1").arg(code, 0, 16);
		}
	}
}

void Diagnostics::WriteStartupDiagnostics()
{
	const QString logPath = "C:/ProgramData/AMS/diagnostics_startup.log";

	QFile file(logPath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
		return;

	QTextStream out(&file);
	out << "==== Startup Diagnostics (" << QDateTime::currentDateTime().toString(Qt::ISODate) << ") ====\n";
	out << "OS: " << QSysInfo::prettyProductName() << "\n";
	out << "Kernel: " << QSysInfo::kernelVersion() << "\n";
	out << "CPU-Architecture (Qt): " << QSysInfo::currentCpuArchitecture() << "\n";
	out << "CPU-Architecture (Build): " << getArchitecture() << "\n";
	out << "Qt-Version: " << QT_VERSION_STR << "\n";

	out << "\n-- DLL paths --\n";
	out << "MSVCP140.dll: " << getModulePath(L"msvcp140.dll") << "\n";
	out << "VCRUNTIME140.dll: " << getModulePath(L"vcruntime140.dll") << "\n";
	out << "QtCore.dll: " << getModulePath(L"Qt6Core.dll") << "\n";
	out << "QtGui.dll: " << getModulePath(L"Qt6Gui.dll") << "\n";

	out << "\n==== End ====\n";
	file.close();
}

void Diagnostics::WriteCrashDiagnostics(EXCEPTION_POINTERS* exceptionInfo)
{
	const QString path = "C:/ProgramData/AMS/diagnostics_crash.log";

	QFile file(path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
		return;

	QTextStream out(&file);
	out << "==== Crash Diagnostics (" << QDateTime::currentDateTime().toString(Qt::ISODate) << ") ====\n";

	if (exceptionInfo && exceptionInfo->ExceptionRecord)
	{
		const DWORD code = exceptionInfo->ExceptionRecord->ExceptionCode;
		const ULONG_PTR addr = (ULONG_PTR)exceptionInfo->ExceptionRecord->ExceptionAddress;

		out << "Exception Code: " << exceptionCodeToString(code) << "\n";
		out << "Exception Address: 0x" << QString::number(addr, 16).toUpper() << "\n";
	}

	out << "\n-- DLL paths --\n";
	out << "MSVCP140.dll: " << getModulePath(L"msvcp140.dll") << "\n";
	out << "VCRUNTIME140.dll: " << getModulePath(L"vcruntime140.dll") << "\n";
	out << "QtCore.dll: " << getModulePath(L"Qt6Core.dll") << "\n";
	out << "QtGui.dll: " << getModulePath(L"Qt6Gui.dll") << "\n";

	out << "\n==== End ====\n";
	file.close();
}

void Diagnostics::RedirectStdErrToFile()
{
	const QString logPath = "C:/ProgramData/AMS/exception";
	QDir().mkpath(logPath);

	SYSTEMTIME localTime;
	GetLocalTime(&localTime);

	std::stringstream logFileNameStream;
	logFileNameStream << "critical_error_log_ams_"
		<< localTime.wYear << "-"
		<< localTime.wMonth << "-"
		<< localTime.wDay << ".log";

	const std::string logFileName = logFileNameStream.str();
	const std::string logFile = logPath.toStdString() + "/" + logFileName;

	FILE* file = freopen(logFile.c_str(), "a", stderr);
	if (!file)
		std::cerr << "Failed to redirect stderr to log file: " << logFile << '\n';
}

static LONG WINAPI CustomUnhandledExceptionFilter(EXCEPTION_POINTERS* pExceptionPointers)
{
	Diagnostics::WriteCrashDiagnostics(pExceptionPointers);
	CrashHandler::WriteDump(pExceptionPointers);

#ifndef _DEBUG
	if (QCoreApplication::instance())
	{
		QMessageBox::critical(nullptr, QObject::tr("Error"),
			QObject::tr("A fatal error has occurred and the program must be terminated.\n"
			"An error report has been saved. Please contact support."));
	}
#endif


	std::cerr << "Unhandled exception occurred. Crash dump written." << '\n';

	return EXCEPTION_EXECUTE_HANDLER;
}

static void SehTranslator(unsigned int code, EXCEPTION_POINTERS* info)
{
	throw SehException(code, info ? info->ExceptionRecord->ExceptionAddress : nullptr);
}

void Diagnostics::InitSehExceptionHandling()
{
	_set_se_translator(SehTranslator);
	SetUnhandledExceptionFilter(CustomUnhandledExceptionFilter);
}

static void customMessageHandler(QtMsgType type, const QMessageLogContext& /*context*/, const QString& msg)
{
	const QString basePath = "C:/ProgramData/AMS/qtLog";
	const QString dateFolder = QDate::currentDate().toString("yyyy-MM-dd");
	QString fullLogDir = basePath + "/" + dateFolder;

	QDir().mkpath(fullLogDir);
	const QString logFilePath = fullLogDir + "/qtlog.txt";

	QFile file(logFilePath);
	if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
	{
		QTextStream stream(&file);
		QString prefix;

		switch (type)
		{
			case QtDebugMsg:    prefix = "Debug"; break;
			case QtInfoMsg:     prefix = "Info"; break;
			case QtWarningMsg:  prefix = "Warning"; break;
			case QtCriticalMsg: prefix = "Critical"; break;
			case QtFatalMsg:    prefix = "Fatal"; break;
		}

		stream << "[" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "] ";
		stream << prefix << ": " << msg << "\n";
		file.flush();

		if (type == QtFatalMsg)
			abort();
	}
}

void Diagnostics::InitQtMessageHandler()
{
	qInstallMessageHandler(customMessageHandler);
}

// ==== SehException Implementation ====

SehException::SehException(unsigned int code, void* address) : _code(code), _address(address)
{
	std::ostringstream oss;
	oss << "SEH Exception [0x" << std::hex << _code << "]: ";

	switch (_code)
	{
		case EXCEPTION_ACCESS_VIOLATION:        oss << "Access Violation"; break;
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:   oss << "Array Bounds Exceeded"; break;
		case EXCEPTION_DATATYPE_MISALIGNMENT:   oss << "Datatype Misalignment"; break;
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:      oss << "Float Divide by Zero"; break;
		case EXCEPTION_INT_DIVIDE_BY_ZERO:      oss << "Integer Divide by Zero"; break;
		case EXCEPTION_ILLEGAL_INSTRUCTION:     oss << "Illegal Instruction"; break;
		default:                                oss << "Unknown SEH Exception"; break;
	}

	if (_address)
		oss << " at address 0x" << _address;

	_message = oss.str();
}

const char* SehException::what() const noexcept { return _message.c_str(); }
unsigned int SehException::code() const noexcept { return _code; }
void* SehException::address() const noexcept { return _address; }
