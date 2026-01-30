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

void DeleteArticleThread::run()
{
	if (!m_tableWidget)
		return;

	for (const QTableWidgetSelectionRange& selectedRange : m_selectedRanges)
	{
		for (int row = selectedRange.topRow(); row <= selectedRange.bottomRow(); ++row)
		{
			QVariant value = m_tableWidget->item(row, 0)->data(Qt::DisplayRole);
			uint32_t articleID = Util::ConvertVariantToUInt32(value);
	//		m_articleManager->DeleteArticleFromDatabase(articleID);
		}
	}

	emit finished();
}
