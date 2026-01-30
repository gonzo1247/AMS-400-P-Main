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

#pragma once

#include <QTableView>
#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <functional>

namespace TableHelper
{
	inline void SelectFirstRowIfSingleResult(QTableView* view,
		QAbstractItemModel* model,
		const std::function<void(const QModelIndex&)>& callback)
	{
		if (!view || !model)
			return;

		if (model->rowCount() == 1)
		{
			QModelIndex index = model->index(0, 0);
			if (!index.isValid())
				return;

			// Mark the line as selected
			view->setCurrentIndex(index);
			view->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);

			// Call callback
			callback(index);
		}
	}
}
