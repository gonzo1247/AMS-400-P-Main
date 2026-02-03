/*
 * Project Name: Inventory Management System (IMS)
 *
 * Copyright (C) 2023 - 2025 Severin Weitz, Lukas Winter | WeWi-Systems
 *
 * This software is intended for internal use only.
 *
 * License:
 * - This project uses components licensed under the GNU Lesser General Public License (LGPL) v3.
 * - The IMS itself is licensed under LGPL v3 unless otherwise stated.
 * - Third-party libraries and frameworks are used under their respective licenses.
 *
 * External Libraries Used:
 * - Qt 6.8.3 (LGPL v3) (dynamic linking)
 * - KDReports (GPL v2)
 * - MySQL Connector/C++ (GPL v2 + FOSS Exception)
 * - OpenCV (Apache License 2.0)
 * - OpenSSL (Apache License 2.0)
 * - QXlsx (MIT License)
 * - ZXing-C++ (Apache License 2.0)
 *
 * Full license texts and third-party license information can be found in LICENSE.txt and LICENSES_THIRD_PARTY.txt.
 *
 * For more information about Qt licensing, see: https://www.qt.io/licensing/
 *
 * Author: Severin Weitz, Lukas Winter
 * Date: 07.10.2024
 */

#include "AMSMain.h"
#include <QDebug>
#include <QFile>
#include <QLabel>
#include <QLocale>
#include <QPixmap>
#include <QTextStream>
#include <QtGlobal>
#include <QTimer>
#include <QTranslator>
#include <QApplication>
#include <QThread>
#include <QDir>
#include <QString>
#include <QLineEdit>
#include <QTextEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QDateEdit>
#include <QTimeEdit>
#include <QDateTimeEdit>
#include <QPlainTextEdit>

#include <windows.h>
#include <eh.h> // For _set_se_translator
#include <exception>
#include <dbghelp.h>
#include <shobjidl_core.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <locale>
#include <string>

#include "CrashHandler.h"
#include "CrashReport.h"
#include "Diagnostics.h"
#include "Settings.h"
#include "DatabaseConnection.h"
#include "MainFrame.h"
#include "MySQLPreparedStatements.h"
#include "PasswordStore.h"
#include "SettingsManager.h"
#include "SettingsMigrator.h"
#include "SqlValidator.h"
#include "Databases.h"
#include "FileKeyProvider.h"
#include "MySQLPreparedStatements.h"
#include "WinStackTrace.h"
#include "ShutdownManager.h"

void LoadAndStartSQL();

int main(int argc, char* argv[])
{
	// Set SEH translator and custom exception handler
	Diagnostics::InitSehExceptionHandling();

	std::locale::global(std::locale("de_DE.UTF-8"));

	SettingsManager::instance().InitDefaults();
    SettingsManager::instance().SynSettings();
	SettingsManager::instance().LoadSettings();

	Diagnostics::RedirectStdErrToFile();

#ifdef _DEBUG
	Diagnostics::InitQtMessageHandler();
	qputenv("QT_DEBUG_PLUGINS", QByteArray("1"));
#endif	

	QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    #ifdef Q_OS_WIN
    SetCurrentProcessExplicitAppUserModelID(L"WeWi-Systems.AMS");
#endif

	QApplication mApplication(argc, argv);

	QCoreApplication::setOrganizationName("WeWiSystems");
    QCoreApplication::setApplicationName("AMS");

	Logger::Init();
	ShutdownManager::Instance().Initialize(&mApplication);
	std::atexit([]() { ShutdownManager::Instance().OnProcessExit(); });

	try
	{
		Diagnostics::WriteStartupDiagnostics();

        LoadAndStartSQL();
		
		const auto _mainFrame = std::make_shared<MainFrame>(mApplication);

		MainFrameSingelton::getInstance().setMainFramePointer(_mainFrame);

        SettingsManager::instance().LoadGlobalSettings();

        FileKeyProvider::Init();

#ifdef _DEBUG
		std::thread([] {
			SqlValidator::ValidateAllStatements();
			}).detach();
#endif

		const QIcon appIcon(":/icons/resources/icons/AMS4.png");   // multi-size .ico: 16..256 px
		mApplication.setWindowIcon(appIcon);									// default for all top-level windows

		const int result = _mainFrame->StartFrameAndProgramm(argc, argv);
		ShutdownManager::Instance().OnMainReturning(result);
		return result;
	}
	catch (const SehException& e)
	{
		std::cerr << "SEH Exception: " << e.what() << '\n';
        WriteExceptionReportToFile("SehException", "SEH Exception", 1, 64);
		LOG_MISC("Main: SEH Exception: {}", e.what());
        LogCurrentStackTrace(0, 64);
		return EXIT_FAILURE;
	}
    catch (const std::runtime_error& e)
    {
        WriteExceptionReportToFile("std::runtime_error", "Runtime error", 1, 64);
        LOG_MISC("Main: Runtime error: {}", e.what());
        return EXIT_FAILURE;
    }
    catch (const std::bad_variant_access& e)
    {
        WriteExceptionReportToFile("std::bad_variant_access", e.what(), 1, 64);
        LOG_MISC("Main: Bad variant access");
        return EXIT_FAILURE;
    }
	catch (const std::exception& e)
	{
        WriteExceptionReportToFile("std::exception", e.what(), 1, 64);
		LOG_MISC("Main: Uncaught exception: {}", e.what());
		return EXIT_FAILURE;
	}
	catch (...)
	{
        WriteExceptionReportToFile("unknown", "unknown exception", 1, 64);
		LOG_MISC("Main: Uncaught unknown exception.");
		return EXIT_FAILURE;
	}
}

void LoadAndStartSQL()
{
    RegisterPreparedStatements();


    PoolConfig imsConfig;
    imsConfig.primary = GetSettings().getMySQLSettings();
    imsConfig.syncLimits = {.minSize = 2, .maxSize = 10, .maxQueueDepth = 2048};
    imsConfig.asyncLimits = {.minSize = 2, .maxSize = 10, .maxQueueDepth = 2048};
    IMSDatabase::Configure(imsConfig);

    PoolConfig amsConfig;
    amsConfig.primary = GetSettings().getAMSMySQLSettings();
    amsConfig.syncLimits = {.minSize = 2, .maxSize = 10, .maxQueueDepth = 2048};
    amsConfig.asyncLimits = {.minSize = 2, .maxSize = 10, .maxQueueDepth = 2048};
    AMSDatabase::Configure(amsConfig);
};
