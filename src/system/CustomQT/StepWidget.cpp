#include "StepWidget.h"

#include <QPainter>

using namespace oclero::qlementine;

StepWidget::StepWidget(QWidget* parent) : QWidget(parent)
{
    _badge = new StatusBadgeWidget(this);
    _badge->setBadgeSize(StatusBadgeSize::Medium);

    _bgColor = QColor(255, 0, 0, 40);
}

void StepWidget::setBadge(StatusBadge badge)
{
    _badge->setBadge(badge);
    update();
}

void StepWidget::setStepColor(const QColor& c)
{
    _bgColor = c;
    update();
}

void StepWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);


    p.setBrush(_bgColor);
    p.setPen(Qt::NoPen);

    const int radius = 12;
    p.drawRoundedRect(rect(), radius, radius);

    const QSize sz = _badge->sizeHint();
    const int x = (width() - sz.width()) / 2;
    const int y = (height() - sz.height()) / 2;
    _badge->setGeometry(x, y, sz.width(), sz.height());
}
