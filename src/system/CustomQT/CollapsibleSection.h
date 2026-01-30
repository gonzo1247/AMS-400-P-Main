#pragma once

#include <QWidget>

class QToolButton;

class CollapsibleSection : public QWidget
{
    Q_OBJECT

   public:
    explicit CollapsibleSection(const QString& title, QWidget* content, QWidget* parent = nullptr);

    bool isExpanded() const;
    void setExpanded(bool expanded);

   signals:
    void expandedChanged(bool expanded);

   private:
    QToolButton* _toggleButton = nullptr;
    QWidget* _content = nullptr;

    void UpdateUi();
};
