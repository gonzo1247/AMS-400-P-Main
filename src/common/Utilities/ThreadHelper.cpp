/*
 * Copyright (C) 2023 - 2025 Severin Weitz, Lukas Winter | WeWi-Systems
 *
 * This file is part of the Inventory Management System project,
 * licensed under the GNU Lesser General Public License (LGPL) v3.
 *
 * For more details, see the full license header in main.cpp or
 * InventoryManagementSystem.cpp and the LICENSE.txt file.
 *
 * Author: Severin Weitz, Lukas Winter
 * Date: [07.10.2024]
 */

#include "ThreadHelper.h"

std::vector<std::uint32_t> DeleteArticleThread::CollectArticleIds(QTableWidget* tableWidget, const QList<QTableWidgetSelectionRange>& ranges)
{
    std::vector<std::uint32_t> ids;

    if (!tableWidget)
        return ids;

    for (const QTableWidgetSelectionRange& selectedRange : ranges)
    {
        for (int row = selectedRange.topRow(); row <= selectedRange.bottomRow(); ++row)
        {
            const auto item = tableWidget->item(row, 0);
            if (!item)
                continue;

            const QVariant value = item->data(Qt::DisplayRole);
            ids.push_back(Util::ConvertVariantToUInt32(value));
        }
    }

    return ids;
}

void DeleteArticleThread::run()
{
    for (const auto articleID : m_articleIds)
    {
        (void)articleID;
        //	m_articleManager->DeleteArticleFromDatabase(articleID);
    }

	emit finished();
}
