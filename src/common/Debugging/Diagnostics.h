#pragma once

#include <exception>
#include <string>
#include <Windows.h>

#include <exception>
#include <string>
#include <iostream>
#include <QDir>

class Diagnostics
{
public:
	static void WriteStartupDiagnostics();
	static void WriteCrashDiagnostics(EXCEPTION_POINTERS* exceptionInfo);

	static void RedirectStdErrToFile();

	static void InitSehExceptionHandling();

	static void InitQtMessageHandler();
};

class SehException : public std::exception
{
public:
	SehException(unsigned int code, void* address = nullptr);
	const char* what() const noexcept override;
	unsigned int code() const noexcept;
	void* address() const noexcept;

private:
	unsigned int _code;
	void* _address;
	std::string _message;
};