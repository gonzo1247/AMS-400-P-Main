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

#include <QObject>
#include <QList>
#include <QTableWidget>
#include <QThread>
#include <QTableWidget>
#include <QVariant>
#include <cstdint>
#include <vector>

#include "Util.h"

class ArticleManager;

class DeleteArticleThread : public QThread
{
	Q_OBJECT
public:
	explicit DeleteArticleThread(QObject* parent = nullptr) : QThread(parent) {}

	void setArticleManager(ArticleManager* manager) { m_articleManager = manager; }
    void setArticleIds(const std::vector<std::uint32_t>& ids) { m_articleIds = ids; }
    void setArticleIds(std::vector<std::uint32_t>&& ids) { m_articleIds = std::move(ids); }
    static std::vector<std::uint32_t> CollectArticleIds(QTableWidget* tableWidget, const QList<QTableWidgetSelectionRange>& ranges);


protected:
	void run() override;

signals:
	void finished();

private:
	ArticleManager* m_articleManager = nullptr;
    std::vector<std::uint32_t> m_articleIds;
};

class ThreadHelper
{
};

