#pragma once

// Touch/keyboard detection helpers (Windows-only).

// Returns true if the device reports touch capability.
bool hasTouch();

// Returns true if convertible is in slate/tablet mode.
// Windows returns 0 in slate mode, non-zero otherwise.
bool isSlateMode();

// Returns true if any physical keyboard (USB etc.) is present.
bool hasHardwareKeyboard();

// Policy: show soft keyboard if slate mode OR (touch and no physical keyboard).
inline bool allowSoftKeyboard()
{
	return isSlateMode() || (hasTouch() && !hasHardwareKeyboard());
}
