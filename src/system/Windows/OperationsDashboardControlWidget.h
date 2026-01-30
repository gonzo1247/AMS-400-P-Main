#pragma once

#include <QMainWindow>
#include "ui_OperationsDashboardControlWidget.h"
#include "OperationsDashboardWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
    class OperationsDashboardControlWidgetClass;
};
QT_END_NAMESPACE

class OperationsDashboardControlWidget : public QMainWindow
{
    Q_OBJECT

   public:
    explicit OperationsDashboardControlWidget(OperationsDashboardWidget* dashboardWidget, QWidget* parent = nullptr);
    ~OperationsDashboardControlWidget() override;

   private:
    Ui::OperationsDashboardControlWidgetClass* ui = nullptr;

    OperationsDashboardWidget* _operationsDashboard;

    void ShowTargetOnSelectedScreenFullscreen();
    void UpdateUI();
    void LoadUiFromDashboard();

    void ApplySpeedDownPxPerSec(double px_per_sec);
    void ApplySpeedUpPxPerSec(double px_per_sec);
    void ApplyPauseTopMs(int ms);
    void ApplyPauseBottomMs(int ms);

    double GetSpeedDownPxPerSec() const;
    double GetSpeedUpPxPerSec() const;
    int GetPauseTopMs() const;
    int GetPauseBottomMs() const;

    void ApplyAllFromUi();

    private slots:
     void onPushIncrease();
     void onPushDecrease();
     void RefreshScreens();

};
