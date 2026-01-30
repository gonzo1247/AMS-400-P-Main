#include "pch.h"
#include "CollapsibleSection.h"

#include <QToolButton>
#include <QVBoxLayout>

CollapsibleSection::CollapsibleSection(const QString& title, QWidget* content, QWidget* parent) : QWidget(parent), _content(content)
{
    _toggleButton = new QToolButton(this);
    _toggleButton->setText(title);
    _toggleButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    _toggleButton->setArrowType(Qt::RightArrow);
    _toggleButton->setCheckable(true);
    _toggleButton->setChecked(false);

    _content->setParent(this);
    _content->setVisible(false);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    layout->addWidget(_toggleButton);
    layout->addWidget(_content);

    connect(_toggleButton, &QToolButton::toggled, this, [this](bool checked) { setExpanded(checked); });
}

bool CollapsibleSection::isExpanded() const { return _toggleButton->isChecked(); }

void CollapsibleSection::setExpanded(bool expanded)
{
    if (_toggleButton->isChecked() != expanded)
        _toggleButton->setChecked(expanded);

    _content->setVisible(expanded);
    UpdateUi();

    emit expandedChanged(expanded);
}

void CollapsibleSection::UpdateUi() { _toggleButton->setArrowType(isExpanded() ? Qt::DownArrow : Qt::RightArrow); }
