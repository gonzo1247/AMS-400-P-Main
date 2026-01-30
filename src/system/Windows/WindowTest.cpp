#include "WindowTest.h"

WindowTest::WindowTest(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::WindowTestClass())
{
	ui->setupUi(this);
}

WindowTest::~WindowTest()
{
	delete ui;
}

