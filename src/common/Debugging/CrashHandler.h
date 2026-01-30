#pragma once

#include <Windows.h>

class CrashHandler
{
public:
	static void WriteDump(EXCEPTION_POINTERS* exceptionPointers);
};

