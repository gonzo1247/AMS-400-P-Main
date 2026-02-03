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

#include <QFile>
#include <QList>
#include <QMovie>
#include <QProcess>
#include <QTableWidget>
#include <QThread>
#include <QVBoxLayout>
#include <QWidget>
#include <QCheckBox>

#include "CrashDumpRotator.h"
#include "MainFrame.h"
#include "Miscellaneous/Version.h"
#include "AMSMain.h"
#include "CompanyLocationHandler.h"
#include "CostUnitDataHandler.h"
#include "RBACAccess.h"
#include "SettingsManager.h"
#include "VersionCheck.h"
#include "oclero/qlementine/icons/QlementineIcons.hpp"
#include "DatabaseHealth.h"

MainFrame::MainFrame(QApplication& application) : _isTranslationActive(false), _isStyleChangeActive(false), /*_styleMgr(std::make_unique<StyleManager>()),*/ _loadingScreenTimer(std::make_unique<QTimer>()),
_application(application)
{
	_translator = nullptr;
	_statusLabel = nullptr;
	_progressBar = nullptr;
}

MainFrame::~MainFrame()
{
	delete _translator;
}

int MainFrame::StartFrameAndProgramm(int /*argc*/, char* /*argv*/[])
{
	std::unique_ptr<QSplashScreen> splash = CreateSplashScreen();

	splash->show();
	QApplication::processEvents();

	updateLoadingProgress(10, "Loading translation...");
	LoadTranslation();
	QApplication::processEvents();


	if (!AreDatabasesReachable(GetSettings().getMySQLSettings(), GetSettings().getAMSMySQLSettings()))
	{
		updateLoadingProgress(20, "No MySQL connection found");
		return OnlySettingsWindow();
	}
	
	updateLoadingProgress(25, "Check Database Version information...");

	VersionCheck version;

	if (!version.CheckCompatibilityWithDB(splash.get()))
	{
		// Close splash, open settings, do not schedule more UI work
		splash->finish(nullptr);
		return OnlySettingsWindow();
	}

	updateLoadingProgress(30, "Loading style...");

	QTimer::singleShot(200, this, [this]() {
		initQlementineStyle();
		ChangeTheme(static_cast<Styles>(GetSettings().getStyleSelection()));
		});


	QApplication::processEvents();

	updateLoadingProgress(40, "Initializing access control...");
	QApplication::processEvents();
	CostUnitDataHandler::instance().Initialize();
	CompanyLocationHandler::instance().Initialize();

	updateLoadingProgress(50, "Creating main window...");
	const std::shared_ptr<AMSMain> _mainWindow = std::make_unique<AMSMain>();
	QApplication::processEvents();

    RotateCrashDumps();

	QTimer::singleShot(10, [&]()
		{
			updateLoadingProgress(60, "Loading constructor data...");
	//		_mainWindow->LoadConstructorData();
			QApplication::processEvents();
			QTimer::singleShot(300, [&]()
				{
					updateLoadingProgress(100, "Starting application...");
					_mainWindow->show();
					splash->finish(_mainWindow.get());
				});
		});

/*
	const auto _localComServer = std::make_unique<LocalServer>();
	_localComServer->StartComServer();*/

	QCoreApplication::processEvents();		

	int result = _application.exec();
	
	return result;
}

void MainFrame::LoadPersonalStyle(Styles style)
{
	ChangeTheme(style);
}

void MainFrame::LoadPersonalTranslation(std::string languageString)
{
	QTranslator* newTranslator = new QTranslator();
	SetIsTranslationActive(true);

	if (newTranslator->load(":/translation/translation/Translation_" + QString::fromStdString(languageString) + ".qm"))
	{
		if (_translator != nullptr)
		{
			_application.removeTranslator(_translator);
			delete _translator;
		}

		_application.installTranslator(newTranslator);
		_translator = newTranslator;

		QStringList localeParts = QString::fromStdString(languageString).split('_');

		if (localeParts.size() == 2)
		{
			QString language = localeParts.at(0);
			QString country = localeParts.at(1);

			// Create QLocale using explicit language and country codes
			QLocale locale(language + "_" + country);

			if (language == "jp" && country == "JP")
				locale = QLocale(QLocale::Japanese, QLocale::Japan);  // Special handling for jp_JP

			QLocale::setDefault(locale);

			// Construct the correct path for the translation file
			QString translationFilePath = ":/translation/translation/Translation_" + locale.name() + ".qm";

			if (language == "jp" && country == "JP")
				translationFilePath = ":/translation/translation/Translation_" + QString::fromStdString(languageString) + ".qm";

			qApp->addLibraryPath(translationFilePath);
		}
	}
	else
	{
		LOG_MISC("Load Personal Translation for User {} can not load Translation for Language {}", GetUser().GetUserName(), languageString);
		delete newTranslator;
		return;
	}

	UpdateUI();
	SetIsTranslationActive(false);
}

void MainFrame::ResetStyleAndLanguageToDefault()
{
	LoadTranslation();
	ChangeTheme(static_cast<Styles>(GetSettings().getStyleSelection()));
}

void MainFrame::AbortProgrammOnMySQLConnectionError()
{
	LOG_SQL("ERROR: MySQL Connection lost and can not restore. Shut down Programm.");
	_application.quit();
}

/********************* Private Functions ************************************/
void MainFrame::LoadTranslation()
{
	QTranslator* translator = new QTranslator();
	_translator = translator;
	const std::string languageCode = GetSettings().getLanguage();
	const QStringList localeParts = QString::fromStdString(languageCode).split('_');

	if (localeParts.size() == 2)
	{
		const QString language = localeParts.at(0);
		const QString country = localeParts.at(1);

		// Create QLocale using explicit language and country codes
		QLocale locale(language + "_" + country);

		if (language == "jp" && country == "JP")
			locale = QLocale(QLocale::Japanese, QLocale::Japan);  // Special handling for jp_JP

		QLocale::setDefault(locale);

		// Construct the correct path for the translation file
		QString translationFilePath = ":/translation/translation/Translation_" + locale.name() + ".qm";

		if (language == "jp" && country == "JP")
			translationFilePath = ":/translation/translation/Translation_" + QString::fromStdString(languageCode) + ".qm";

		qApp->addLibraryPath(translationFilePath);

		if (translator->load(translationFilePath))
			qApp->installTranslator(translator);
	}
}

void MainFrame::initQlementineStyle()
{
	if (!style_)
	{
		style_ = new oclero::qlementine::QlementineStyle(qApp);
		style_->setAnimationsEnabled(true);
		QApplication::setStyle(style_);

		oclero::qlementine::icons::initializeIconTheme();
        QIcon::setThemeName("qlementine");
	}

	if (!themeMgr_)
	{
		themeMgr_ = new oclero::qlementine::ThemeManager(style_);
		themeMgr_->loadDirectory(":/Styles/resources/Styles");
		int test = 0;
	}
}

void MainFrame::ChangeTheme(Styles style)
{

	if (style == Styles::STYLE_DARK)
        themeMgr_->setCurrentTheme("Dark");
    else
        themeMgr_->setCurrentTheme("Light");
}

std::unique_ptr<QSplashScreen> MainFrame::CreateSplashScreen()
{
	// Defines here (change only here)
	QString ProgrammNameShort = "AMS";
	QString ProgrammNameLong = "Assignment and Maintenance System";
	QString Copyright = "\u00A9 2023 - 2025 Severin Weitz, Lukas Winter";
	QString logoPath = ":/icons/resources/icons/lager.png";
	//End of defines

	QPixmap splashPixmap(600, 350);
	splashPixmap.fill(QColor("#037BFC")); // bright-ish modern blue

	std::unique_ptr<QSplashScreen> splash = std::make_unique<QSplashScreen>(splashPixmap);
	splash->setWindowFlag(Qt::WindowStaysOnTopHint, true);
	splash->setWindowFlag(Qt::FramelessWindowHint, true);

	QVBoxLayout* mainLayout = new QVBoxLayout(splash.get());
	mainLayout->setContentsMargins(20, 20, 20, 20);

	QHBoxLayout* topLayout = new QHBoxLayout();

	QLabel* logoLabel = new QLabel(splash.get());
	QPixmap logo(logoPath);
	logoLabel->setPixmap(logo.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));

	QVBoxLayout* versionLayout = new QVBoxLayout();

	QLabel* versionLabel = new QLabel(splash.get());  
	versionLabel->setStyleSheet("color: white; font-weight: bold; font-size: 12px;");
	versionLabel->setText(
		QString::fromLatin1("v")
		+ QString::fromLatin1(VERSION_NUMBER)
		+ QString::fromLatin1(" | ")
		+ QString::fromLatin1(VERSION_DATE)
	);

	QLabel* releaseLabel = new QLabel(splash.get());
	releaseLabel->setStyleSheet("color: white; font-size: 11px;");
	releaseLabel->setText(QString::fromLatin1(BUILD_STATUS " - " VERSION_INFO));

	versionLayout->addWidget(versionLabel, 0, Qt::AlignRight);
	versionLayout->addWidget(releaseLabel, 0, Qt::AlignRight);

	topLayout->addWidget(logoLabel, 0, Qt::AlignLeft);
	topLayout->addLayout(versionLayout, 1);

	mainLayout->addLayout(topLayout);

	QLabel* shortTitle = new QLabel(splash.get());
	shortTitle->setText(ProgrammNameShort);
	shortTitle->setStyleSheet("color: white; font-size: 32px; font-weight: bold;");
	shortTitle->setAlignment(Qt::AlignCenter);
	mainLayout->addWidget(shortTitle);

	QLabel* centerTitle = new QLabel(splash.get());
	centerTitle->setText(ProgrammNameLong);
	centerTitle->setStyleSheet("color: white; font-size: 20px;");
	centerTitle->setAlignment(Qt::AlignCenter);
	mainLayout->addWidget(centerTitle, 1);

	_statusLabel = new QLabel(splash.get());
	_statusLabel->setStyleSheet("color: white; font-size: 12px; margin-bottom: 5px;");
	_statusLabel->setAlignment(Qt::AlignCenter);
	mainLayout->addWidget(_statusLabel);

	_progressBar = new QProgressBar(splash.get());
	_progressBar->setStyleSheet(
		"QProgressBar { "
		"  background-color: #E0E0E0; "
		"  border: 1px solid #AAA; "
		"  border-radius: 6px; "
		"  max-height: 20px; "
		"} "
		"QProgressBar::chunk { "
		"  background: qlineargradient("
		"      x1:0, y1:0.5, x2:1, y2:0.5, "
		"      stop:0 #FFE37F, stop:1 #FFC107"
		"  ); "
		"  border-radius: 6px; "
		"  margin: 1px; "
		"}"
	);
	_progressBar->setRange(0, 100);
	_progressBar->setTextVisible(false);
	_progressBar->setStyleSheet(
		"QProgressBar { "
		"  background-color: #FFFFFF; "
		"  border: 1px solid grey; border-radius: 4px; "
		"  max-height: 20px; "
		"} "
		"QProgressBar::chunk { background-color: #D7D7D7; }"
	);
	_progressBar->setValue(0);
	mainLayout->addWidget(_progressBar);

	// Bottom: Copyright
	QLabel* copyrightLabel = new QLabel(splash.get());
	copyrightLabel->setStyleSheet("color: white; font-size: 10px;");
	copyrightLabel->setText(Copyright);
	copyrightLabel->setAlignment(Qt::AlignCenter);
	mainLayout->addWidget(copyrightLabel);

	// Center the splash on screen
	splash->setGeometry(
		QStyle::alignedRect(
			Qt::LeftToRight,
			Qt::AlignCenter,
			splash->size(),
			QApplication::primaryScreen()->geometry()
		)
	);

	return splash;
}

void MainFrame::updateLoadingProgress(unsigned int progress, const std::string& message)
{
	if (_progressBar)
	{
		_progressBar->setValue(static_cast<int>(progress));
	}
	if (_statusLabel)
	{
		_statusLabel->setText(QString::fromStdString(message));
	}
	QApplication::processEvents();
}

void MainFrame::setTableWidgetsStyle(const QString& style, Styles styleEnum)
{
	QList<QWidget*> topLevelWidgets = _application.topLevelWidgets();

	for (QWidget* topLevelWidget : topLevelWidgets)
	{
		QList<QTableWidget*> tableWidgets = topLevelWidget->findChildren<QTableWidget*>();

		for (QTableWidget* tableWidget : tableWidgets)
		{
			tableWidget->setStyleSheet(style);
		//	_styleMgr->SetTableHeaderFormatAndTableColor(*tableWidget, styleEnum);
		}
	}
}

void MainFrame::UpdateUI()
{
	QList<QWidget*> allWidgets = _application.allWidgets();
	for (auto widget : allWidgets) 
	{
		
		if (auto label = qobject_cast<QLabel*>(widget)) {
			
			label->setText(tr(label->text().toStdString().c_str()));
		}
		else if (auto button = qobject_cast<QPushButton*>(widget)) {
			
			button->setText(tr(button->text().toStdString().c_str()));
		}
		else if (auto checkBox = qobject_cast<QCheckBox*>(widget)) {
			
			checkBox->setText(tr(checkBox->text().toStdString().c_str()));
		}
		else if (auto comboBox = qobject_cast<QComboBox*>(widget)) {
			
			for (int i = 0; i < comboBox->count(); ++i) {
				comboBox->setItemText(i, tr(comboBox->itemText(i).toStdString().c_str()));
			}
		}		
	}
}

int MainFrame::OnlySettingsWindow()
{
	QThread::sleep(2);

	const QString appDir = QCoreApplication::applicationDirPath();
	const QString exeName = QStringLiteral("WeWi-Systems-Setting-Manager.exe");
	const QString exePath = QDir(appDir).filePath(exeName);

	// Auto-detect caller (IMS/OMS/OPS)
	const QString caller = QFileInfo(QCoreApplication::applicationFilePath()).baseName();

	// Validate presence next to IMS.exe
	if (!QFileInfo::exists(exePath))
	{
		LOG_DEBUG("Settings Manager not found: {}", exePath.toStdString());
		QMessageBox::critical(
			nullptr,
			tr("Settings Manager"),
			tr("The Settings Manager was not found.\n\nExpected at:\n%1\n\nPlease report this to support.").arg(exePath)
		);
	}

	// Optional: pass a hint to the manager who called it
	QStringList args;
	args << "--caller=IMS";

	qint64 pid = 0;
	const bool ok = QProcess::startDetached(exePath, args, appDir, &pid);
	if (!ok)
	{
		LOG_DEBUG("Failed to start Settings Manager: {}", exePath.toStdString());
		QMessageBox::critical(
			nullptr,
			tr("Settings Manager"),
			tr("Failed to start the Settings Manager.\n\nTried to launch:\n%1\nCaller: %2\n\nPlease report this to support.")
			.arg(exePath, caller)
		);
	}

	return 0;
}

void MainFrame::RotateCrashDumps()
{
    auto crashData = GetSettings().getCrashRotateData();

    if (crashData.isActive)
    {
        const fs::path dumpDir = LR"(C:/ProgramData/Lager-und-Bestellverwaltung/exception)";

        auto fut = CrashDumpRotator::RotateAsyncNonOverlapping(dumpDir, std::chrono::days{ crashData.maxDays }, crashData.maxCrashDumps);
    }
}
