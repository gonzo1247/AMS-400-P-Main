#pragma once

#include <QWidget>
#include "ui_WidgetTest.h"

QT_BEGIN_NAMESPACE
namespace Ui { class WidgetTestClass; };
QT_END_NAMESPACE

class WidgetTest : public QWidget
{
	Q_OBJECT

public:
	WidgetTest(QWidget *parent = nullptr);
	~WidgetTest();

private:
	Ui::WidgetTestClass *ui;
};

