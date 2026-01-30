#pragma once
#include <string>
#include "Util.h"

namespace BootstrapLogger
{
	void Log(const std::string& message);

	template<typename... Args>
	void Log(std::string_view message, Args&&... args)
	{
		Log(Util::FormatMessageUtil(message, std::forward<Args>(args)...));
	}

	std::wstring GetProgramDataPath();
}
