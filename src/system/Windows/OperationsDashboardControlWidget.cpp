#pragma once

#include "OperationsDashboardControlWidget.h"

#include <QComboBox>
#include <QGuiApplication>
#include <QScreen>
#include <QWidget>
#include <QWindow>

OperationsDashboardControlWidget::OperationsDashboardControlWidget(OperationsDashboardWidget* dashboardWidget,
                                                                   QWidget* parent)
    : QMainWindow(parent), ui(new Ui::OperationsDashboardControlWidgetClass), _operationsDashboard(dashboardWidget)
{
    ui->setupUi(this);

    RefreshScreens();

    // Connections
    connect(ui->pb_increase, &QPushButton::clicked, this, &OperationsDashboardControlWidget::onPushIncrease);
    connect(ui->pb_decrease, &QPushButton::clicked, this, &OperationsDashboardControlWidget::onPushDecrease);
    connect(ui->pB_setfullscreen, &QPushButton::clicked, this,
            &OperationsDashboardControlWidget::ShowTargetOnSelectedScreenFullscreen);

    connect(qApp, &QGuiApplication::screenAdded, this, &OperationsDashboardControlWidget::RefreshScreens);
    connect(qApp, &QGuiApplication::screenRemoved, this, &OperationsDashboardControlWidget::RefreshScreens);

    connect(ui->dSB_SpeedDownPxPerSec, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            &OperationsDashboardControlWidget::ApplySpeedDownPxPerSec);

    connect(ui->dSB_SpeedUpPxPerSec, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            &OperationsDashboardControlWidget::ApplySpeedUpPxPerSec);

    connect(ui->sB_PauseTopMs, qOverload<int>(&QSpinBox::valueChanged), this,
            &OperationsDashboardControlWidget::ApplyPauseTopMs);

    connect(ui->sB_PauseBottomMs, qOverload<int>(&QSpinBox::valueChanged), this,
            &OperationsDashboardControlWidget::ApplyPauseBottomMs);

    ui->gB_Status->hide();
    UpdateUI();
    LoadUiFromDashboard();
}

OperationsDashboardControlWidget::~OperationsDashboardControlWidget() { delete ui; }

void OperationsDashboardControlWidget::onPushDecrease()
{
    if (!_operationsDashboard)
        return;

    _operationsDashboard->decreaseTextPointSize();
    UpdateUI();
}
void OperationsDashboardControlWidget::onPushIncrease()
{
    if (!_operationsDashboard)
        return;

    _operationsDashboard->increaseTextPointSize();
    UpdateUI();
}

void OperationsDashboardControlWidget::RefreshScreens()
{
    ui->cB_screens->clear();

    const QList<QScreen*> screens = QGuiApplication::screens();
    for (int i = 0; i < screens.size(); ++i)
    {
        QScreen* s = screens[i];
        const QString label =
            QString("%1 (%2x%3)").arg(s->name()).arg(s->geometry().width()).arg(s->geometry().height());

        ui->cB_screens->addItem(label, i);
    }
}

void OperationsDashboardControlWidget::ShowTargetOnSelectedScreenFullscreen()
{
    const QList<QScreen*> screens = QGuiApplication::screens();
    if (screens.isEmpty() || _operationsDashboard == nullptr)
    {
        return;
    }

    const int idx = ui->cB_screens->currentData().toInt();
    QScreen* screen = screens.value(idx, screens.first());
    if (screen == nullptr)
    {
        return;
    }

    if (!_operationsDashboard)
        return;

    _operationsDashboard->setWindowFlag(Qt::Window, true);
    _operationsDashboard->setWindowState(Qt::WindowNoState);

    // Ensure native handle exists
    (void)_operationsDashboard->winId();

    if (QWindow* w = _operationsDashboard->windowHandle())
    {
        w->setScreen(screen);
    }

    _operationsDashboard->setGeometry(screen->geometry());
    _operationsDashboard->showFullScreen();
    _operationsDashboard->raise();
    _operationsDashboard->activateWindow();
}

void OperationsDashboardControlWidget::UpdateUI()
{
    ui->lb_currentFontSizeData->setText(QString::number(_operationsDashboard->GetTextPointSize()) + " pt");
}

void OperationsDashboardControlWidget::ApplyAllFromUi()
{
    ApplySpeedDownPxPerSec(ui->dSB_SpeedDownPxPerSec->value());
    ApplySpeedUpPxPerSec(ui->dSB_SpeedUpPxPerSec->value());
    ApplyPauseTopMs(ui->sB_PauseTopMs->value());
    ApplyPauseBottomMs(ui->sB_PauseBottomMs->value());
}

void OperationsDashboardControlWidget::ApplySpeedDownPxPerSec(double px_per_sec)
{
    if (!_operationsDashboard)
        return;

    if (_operationsDashboard->_ticketsScroller)
        _operationsDashboard->_ticketsScroller->SetSpeedDownPxPerSec(px_per_sec);

    if (_operationsDashboard->_contractVisitsScroller)
        _operationsDashboard->_contractVisitsScroller->SetSpeedDownPxPerSec(px_per_sec);
}

void OperationsDashboardControlWidget::ApplySpeedUpPxPerSec(double px_per_sec)
{
    if (!_operationsDashboard)
        return;

    if (_operationsDashboard->_ticketsScroller)
        _operationsDashboard->_ticketsScroller->SetSpeedUpPxPerSec(px_per_sec);

    if (_operationsDashboard->_contractVisitsScroller)
        _operationsDashboard->_contractVisitsScroller->SetSpeedUpPxPerSec(px_per_sec);
}

void OperationsDashboardControlWidget::ApplyPauseTopMs(int ms)
{
    if (!_operationsDashboard)
        return;

    if (_operationsDashboard->_ticketsScroller)
        _operationsDashboard->_ticketsScroller->SetPauseTopMs(ms);

    if (_operationsDashboard->_contractVisitsScroller)
        _operationsDashboard->_contractVisitsScroller->SetPauseTopMs(ms);
}

void OperationsDashboardControlWidget::ApplyPauseBottomMs(int ms)
{
    if (!_operationsDashboard)
        return;

    if (_operationsDashboard->_ticketsScroller)
        _operationsDashboard->_ticketsScroller->SetPauseBottomMs(ms);

    if (_operationsDashboard->_contractVisitsScroller)
        _operationsDashboard->_contractVisitsScroller->SetPauseBottomMs(ms);
}

void OperationsDashboardControlWidget::LoadUiFromDashboard()
{
    const QSignalBlocker b1(ui->dSB_SpeedDownPxPerSec);
    const QSignalBlocker b2(ui->dSB_SpeedUpPxPerSec);
    const QSignalBlocker b3(ui->sB_PauseTopMs);
    const QSignalBlocker b4(ui->sB_PauseBottomMs);

    ui->dSB_SpeedDownPxPerSec->setValue(GetSpeedDownPxPerSec());
    ui->dSB_SpeedUpPxPerSec->setValue(GetSpeedUpPxPerSec());
    ui->sB_PauseTopMs->setValue(GetPauseTopMs());
    ui->sB_PauseBottomMs->setValue(GetPauseBottomMs());
}

double OperationsDashboardControlWidget::GetSpeedDownPxPerSec() const
{
    if (!_operationsDashboard)
        return 0.0;

    if (_operationsDashboard->_ticketsScroller)
        return _operationsDashboard->_ticketsScroller->GetSpeedDownPxPerSec();

    if (_operationsDashboard->_contractVisitsScroller)
        return _operationsDashboard->_contractVisitsScroller->GetSpeedDownPxPerSec();

    return 0.0;
}

double OperationsDashboardControlWidget::GetSpeedUpPxPerSec() const
{
    if (!_operationsDashboard)
        return 0.0;

    if (_operationsDashboard->_ticketsScroller)
        return _operationsDashboard->_ticketsScroller->GetSpeedUpPxPerSec();

    if (_operationsDashboard->_contractVisitsScroller)
        return _operationsDashboard->_contractVisitsScroller->GetSpeedUpPxPerSec();

    return 0.0;
}

int OperationsDashboardControlWidget::GetPauseTopMs() const
{
    if (!_operationsDashboard)
        return 0;

    if (_operationsDashboard->_ticketsScroller)
        return _operationsDashboard->_ticketsScroller->GetPauseTopMs();

    if (_operationsDashboard->_contractVisitsScroller)
        return _operationsDashboard->_contractVisitsScroller->GetPauseTopMs();

    return 0;
}

int OperationsDashboardControlWidget::GetPauseBottomMs() const
{
    if (!_operationsDashboard)
        return 0;

    if (_operationsDashboard->_ticketsScroller)
        return _operationsDashboard->_ticketsScroller->GetPauseBottomMs();

    if (_operationsDashboard->_contractVisitsScroller)
        return _operationsDashboard->_contractVisitsScroller->GetPauseBottomMs();

    return 0;
}
