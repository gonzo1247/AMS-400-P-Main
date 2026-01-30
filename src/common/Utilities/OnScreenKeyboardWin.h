#pragma once

#include <QtWidgets/QWidget>

class OnScreenKeyboardWin final
{
public:
	static bool showFor(QWidget* w);
	static void hide();
	static bool isVisible();

private:
	static bool toggleTip(QWidget* w); // impl in .cpp
};
