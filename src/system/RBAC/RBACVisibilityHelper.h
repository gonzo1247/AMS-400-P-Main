#pragma once

#include "PermissionRBAC.h"
#include "RBACAccess.h"

template <typename T>
static void ChangeVisibilityAndTooltip(const Permission& perm, const T& v, const QString& tooltip = "Access denied")
{
    const bool access = RBACAccess::HasPermission(static_cast<std::uint32_t>(perm));
    if constexpr (std::is_same_v<T, QPushButton*>)
    {
        QPushButton* p = v;
        p->setEnabled(access);
        p->setToolTip(access ? tooltip : QString());
    }
    else if constexpr (std::is_same_v<T, QLineEdit*>)
    {
        QLineEdit* p = v;
        p->setEnabled(access);
        p->setToolTip(access ? tooltip : QString());
    }
    else if constexpr (std::is_same_v<T, QComboBox*>)
    {
        QComboBox* p = v;
        p->setEnabled(access);
        p->setToolTip(access ? tooltip : QString());
    }
    else if constexpr (std::is_same_v<T, QTextEdit*>)
    {
        QTextEdit* p = v;
        p->setEnabled(access);
        p->setToolTip(access ? tooltip : QString());
    }
    else
    {
        static_assert(!sizeof(T), "RBACVisibilityHelper: Unsupported widget type");
    }
}
