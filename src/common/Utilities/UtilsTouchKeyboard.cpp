#include "UtilsTouchKeyboard.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <vector>

#pragma comment(lib, "User32.lib")

bool hasTouch()
{
	// Prefer SM_MAXIMUMTOUCHES when available.
	const int maxTouches = GetSystemMetrics(SM_MAXIMUMTOUCHES);
	if (maxTouches > 0)
		return true;

	// Fallback: SM_DIGITIZER with NID_READY (0x80).
	const int digitizer = GetSystemMetrics(SM_DIGITIZER);
	return (digitizer & 0x80) != 0;
}

bool isSlateMode()
{
	// 0 = slate/tablet, non-zero = laptop mode.
	return GetSystemMetrics(0x2003 /* SM_CONVERTIBLESLATEMODE */) == 0;
}

bool hasHardwareKeyboard()
{
	// Enumerate raw input devices; if any keyboard found, return true.
	UINT nDevices = 0;
	if (GetRawInputDeviceList(nullptr, &nDevices, sizeof(RAWINPUTDEVICELIST)) == (UINT)-1)
		return true; // Be conservative on error.

	if (nDevices == 0)
		return false;

	std::vector<RAWINPUTDEVICELIST> list(nDevices);
	if (GetRawInputDeviceList(list.data(), &nDevices, sizeof(RAWINPUTDEVICELIST)) == (UINT)-1)
		return true;

	for (UINT i = 0; i < nDevices; ++i)
	{
		if (list[i].dwType == RIM_TYPEKEYBOARD)
			return true;
	}
	return false;
}
