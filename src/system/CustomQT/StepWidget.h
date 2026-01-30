#pragma once

#include <QWidget>
#include <oclero/qlementine/style/QlementineStyle.hpp>
#include <oclero/qlementine/widgets/StatusBadgeWidget.hpp>

class StepWidget : public QWidget
{
    Q_OBJECT

   public:
    explicit StepWidget(QWidget* parent = nullptr);

    void setBadge(oclero::qlementine::StatusBadge badge);
    void setStepColor(const QColor& c);

   protected:
    void paintEvent(QPaintEvent*) override;

   private:
    QColor _bgColor;
    oclero::qlementine::StatusBadgeWidget* _badge;
};
