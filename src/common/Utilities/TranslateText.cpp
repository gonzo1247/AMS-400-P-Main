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

#include "TranslateText.h"

QString TranslateText::TranslateString(const char* context, const char* sourceText, const char* disambiguation /*= nullptr*/, int n /*= -1*/)
{
	return QCoreApplication::translate(context, sourceText, disambiguation, n).toUtf8().constData();
}
