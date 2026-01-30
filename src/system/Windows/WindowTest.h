#pragma once

#include <QMainWindow>
#include "ui_WindowTest.h"

QT_BEGIN_NAMESPACE
namespace Ui { class WindowTestClass; };
QT_END_NAMESPACE

class WindowTest : public QMainWindow
{
	Q_OBJECT

public:
	WindowTest(QWidget *parent = nullptr);
	~WindowTest();

private:
	Ui::WindowTestClass *ui;
};

