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

#include <QApplication>
#include <QTranslator>
#include <QObject>
#include <QSplashScreen>
#include <QTimer>
#include <QLabel>
#include <QProgressBar>
#include <oclero/qlementine.hpp>

#include "Settings.h"
#include "SharedDefines.h"

class MainFrame : public QObject
{
	Q_OBJECT
public:
	MainFrame(QApplication& application);
	~MainFrame();

	int StartFrameAndProgramm(int argc, char* argv[]);

	void LoadPersonalStyle(Styles style);
	void LoadPersonalTranslation(std::string languageString);

	void ResetStyleAndLanguageToDefault();

	bool GetIsTranslationActive() const { return _isTranslationActive; }
	void SetIsTranslationActive(bool val) { _isTranslationActive = val; }
	bool GetIsStyleChangeActive() const { return _isStyleChangeActive; }
	void SetIsStyleChangeActive(bool val) { _isStyleChangeActive = val; }

	void AbortProgrammOnMySQLConnectionError();

private:
	void LoadTranslation();
	void initQlementineStyle();
	void ChangeTheme(Styles style);
	std::unique_ptr<QSplashScreen> CreateSplashScreen();


	void setTableWidgetsStyle(const QString& style, Styles styleEnum);
	void UpdateUI();
	void updateLoadingProgress(unsigned int progress, const std::string& message);

	int OnlySettingsWindow();

    void RotateCrashDumps();
	
	bool _isTranslationActive;
	bool _isStyleChangeActive;

/*	std::unique_ptr<StyleManager> _styleMgr;*/
	std::unique_ptr<QTimer> _loadingScreenTimer;
	oclero::qlementine::QlementineStyle* style_ = nullptr;
	oclero::qlementine::ThemeManager* themeMgr_ = nullptr;

	QApplication& _application;
	QTranslator* _translator;

	QLabel* _statusLabel;
	QProgressBar* _progressBar;
};

class MainFrameSingelton
{
public:
	static MainFrameSingelton& getInstance()
	{
		static MainFrameSingelton instance;
		return instance;
	}

	std::shared_ptr<MainFrame> getMainFrameSingeltonPointer() { return _mainFrame; }
	void setMainFramePointer(std::shared_ptr<MainFrame> mainFrame) { _mainFrame = mainFrame; }

private:
	MainFrameSingelton() { }

	std::shared_ptr<MainFrame> _mainFrame;

};

inline std::shared_ptr<MainFrame> GetMainFrameClass()
{
	return MainFrameSingelton::getInstance().getMainFrameSingeltonPointer();
}
