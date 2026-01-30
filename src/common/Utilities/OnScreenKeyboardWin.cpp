#include "OnScreenKeyboardWin.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <objbase.h>
#include <shellapi.h>

// Linker pragmas (alternativ in CMake: Shell32, Ole32)
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Ole32.lib")

// {37C994E7-432B-4834-A2F7-DCE1F13B834B}
static const IID IID_ITipInvocation =
{ 0x37c994e7, 0x432b, 0x4834,{ 0xa2, 0xf7, 0xdc, 0xe1, 0xf1, 0x3b, 0x83, 0x4b } };

// {33C53A50-F456-4884-B049-85FD643ECFED}
static const CLSID CLSID_TipInvocation =
{ 0x33c53a50, 0xf456, 0x4884,{ 0xb0, 0x49, 0x85, 0xfd, 0x64, 0x3e, 0xcf, 0xed } };

// Minimal COM interface
struct ITipInvocation : public IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE Toggle(HWND hwnd) = 0;
};

bool OnScreenKeyboardWin::showFor(QWidget* w)
{
	// 1) Wenn sichtbar, nichts tun
	if (isVisible())
		return true;

	// 2) COM-Toggle (erst mit HWND, dann notfalls global)
	if (toggleTip(w) || toggleTip(nullptr))
		if (isVisible())
			return true;

	// 3) Fallback: TabTip starten (verschiedene Pfade ausprobieren)
	const wchar_t* candidates[] = {
		L"C:\\Program Files\\Common Files\\microsoft shared\\ink\\TabTip.exe",
		L"C:\\Program Files (x86)\\Common Files\\microsoft shared\\ink\\TabTip.exe",
		L"%CommonProgramFiles%\\microsoft shared\\ink\\TabTip.exe"
	};

	for (auto path : candidates)
	{
		HINSTANCE res = ShellExecuteW(nullptr, L"open", path, nullptr, nullptr, SW_SHOW);
		if (reinterpret_cast<intptr_t>(res) > 32)
			break;
	}

	// 4) Kurzen Moment geben und erneut prüfen/TOGGLE
	Sleep(120);
	toggleTip(nullptr);
	Sleep(60);

	return isVisible();
}

void OnScreenKeyboardWin::hide()
{
	// Try COM toggle off
	if (isVisible())
		toggleTip(nullptr);

	// Hard hide as fallback
	if (HWND h = FindWindowW(L"IPTip_Main_Window", nullptr))
		PostMessageW(h, WM_SYSCOMMAND, SC_CLOSE, 0);
}

bool OnScreenKeyboardWin::isVisible()
{
	if (HWND h = FindWindowW(L"IPTip_Main_Window", nullptr))
		return IsWindowVisible(h) != 0;
	return false;
}

bool OnScreenKeyboardWin::toggleTip(QWidget* w)
{
	// Init COM apartment
	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	bool didInit = SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE;

	ITipInvocation* tip = nullptr;
	HRESULT c = CoCreateInstance(CLSID_TipInvocation, nullptr, CLSCTX_INPROC_SERVER,
		IID_ITipInvocation, reinterpret_cast<void**>(&tip));
	if (FAILED(c) || !tip)
	{
		if (didInit && hr == S_OK)
			CoUninitialize();
		return false;
	}

	HWND hwnd = w ? reinterpret_cast<HWND>(w->window()->winId()) : nullptr;
	HRESULT t = tip->Toggle(hwnd);
	tip->Release();

	if (didInit && hr == S_OK)
		CoUninitialize();

	return SUCCEEDED(t);
}
