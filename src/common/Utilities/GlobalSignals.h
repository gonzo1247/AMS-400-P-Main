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

#ifndef GLOBALSIGNALS_H
#define GLOBALSIGNALS_H

#include <QObject>

#include "SharedDefines.h"

class GlobalSignals : public QObject
{
	Q_OBJECT

public:
	static GlobalSignals* instance();

signals:
	void closeSignal();
	void exitFullscreenSignal();

	//Barcode
	void ReceiveBarcodeArticle(std::string barcode);
	void ReceiveBarcodeReturnSlip(std::string barcode);
	void ReceiveBarcodeCostunit(std::string barcode);
	void ReceiveBarcodeMaintenanceOrder(std::string barcode);
	void ReceiveChipDataAndSend(std::string chipData);
	void ReceiveBarcodeControl_tts(std::string barcode);

	void ReconnectChipDevice();
	void ReconnectBarcodeDevice();

	// Logout
    void LogoutSignal();

	// CallerManager
	void SignalReloadCallerTable();

	// EmployeeManager
	void SignalReloadEmployeeTable();

	// Create Ticket Widget
    void CreateTicketBackToMainPage();
	void TicketSaveSuccessfully();
	void TicketSaveFail();

    // File Upload (Ticket Details)
    void FileUploadFinished();

	//Send Statusmessage
    void SendStatusMessage(const std::string& message, StatusMessageQueue queue, const QColor& color);

private:
	GlobalSignals() = default;
	static GlobalSignals* _instance;
};

#endif
