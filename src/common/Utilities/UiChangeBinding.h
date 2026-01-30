#pragma once

#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>

template <typename Tracker, typename FieldEnum, typename OnChanged>
void BindLineEdit(QLineEdit* edit, Tracker& tracker, FieldEnum field, OnChanged onChanged)
{
    QObject::connect(edit, &QLineEdit::textChanged, edit, [&tracker, field, onChanged](const QString& v)
    {
        tracker.SetValue(field, v);
        onChanged();
    });
}

template <typename Tracker, typename FieldEnum, typename OnChanged>
void BindComboBox(QComboBox* box, Tracker& tracker, FieldEnum field, OnChanged onChanged)
{
    QObject::connect(box, QOverload<int>::of(&QComboBox::currentIndexChanged), box, [&tracker, field, onChanged, box](int)
    {
        tracker.SetValue(field, box->currentData());
        onChanged();
    });
}

template <typename Tracker, typename FieldEnum, typename OnChanged>
void BindTextEdit(QTextEdit* edit, Tracker& tracker, FieldEnum field, OnChanged onChanged)
{
    QObject::connect(edit, &QTextEdit::textChanged, edit, [&tracker, field, onChanged, edit]()
    {
        tracker.SetValue(field, edit->toPlainText());
        onChanged();
    });
}
