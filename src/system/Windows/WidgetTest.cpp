#include "WidgetTest.h"

WidgetTest::WidgetTest(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::WidgetTestClass())
{
	ui->setupUi(this);
}

WidgetTest::~WidgetTest()
{
	delete ui;
}

