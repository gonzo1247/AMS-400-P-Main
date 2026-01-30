/*
Copyright 2023 - 2024 by Severin
and TrinityCore
*/

#pragma once

#include "Util.h"

#include <windows.h>

#include <QAbstractItemModel>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMovie>
#include <QSortFilterProxyModel>
#include <QSpinBox>
#include <QTableView>
#include <QThread>
#include <QVariant>
#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdint>
#include <ctime>
#include <format>
#include <limits>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <QRandomGenerator>
#include <QHeaderView>

#include "Logger.h"
#include "ThreadWorker.h"
#include "TranslateText.h"
#include "oclero/qlementine/style/QlementineStyle.hpp"
#include "SharedDefines.h"

Tokenizer::Tokenizer(const std::string& src, const char sep, std::uint32_t vectorReserve /*= 0*/, bool keepEmptyStrings /*= true*/)
{
	m_str = new char[src.length() + 1];
	memcpy(m_str, src.c_str(), src.length() + 1);

	if (vectorReserve)
		m_storage.reserve(vectorReserve);

	char* posold = m_str;
	char* posnew = m_str;

	for (;;)
	{
		if (*posnew == sep)
		{
			if (keepEmptyStrings || posold != posnew)
				m_storage.push_back(posold);

			posold = posnew + 1;
			*posnew = '\0';
		}
		else if (*posnew == '\0')
		{
			// Hack like, but the old code accepted these kind of broken strings,
			// so changing it would break other things
			if (posold != posnew)
				m_storage.push_back(posold);

			break;
		}

		++posnew;
	}
}

bool Util::QStringContainsOnlyDigits(const QString& string)
{
	for (const QChar& ch : string)
		if (!ch.isDigit())
			return false;

	return true;
}

std::uint8_t Util::ConvertStringToUint8(const std::string& str)
{
	try
	{
		const int temp = std::stoi(str);

		if (temp < 0 || temp > 255)
			throw std::invalid_argument("Number out of range.");

		return static_cast<std::uint8_t>(temp);
	}
	catch (const std::invalid_argument& e)
	{
		LOG_MISC("Util::ConvertStringToUint8: Invalid Agrument for String {} Message is: {}", e.what(), str);
		return 0;
	}
	catch (const std::exception& e)
	{
		LOG_MISC("Util::ConvertStringToUint8: Error on converting String {} to Uint8. Expection: {}");
		return 0;
	}
}

std::uint32_t Util::ConvertStringToUint32(const std::string& str)
{
	try
	{
		if (str.empty())
			return 0;

		std::istringstream iss(str);
		std::uint32_t value;

		// Try to parse the string as a uint32_t
		if (!(iss >> value))
		{
			throw std::invalid_argument("The input string cannot be converted to a valid number.");
		}

		// Check if there are trailing characters in the string
		if (!iss.eof())
		{
			throw std::invalid_argument("The input string contains invalid trailing characters.");
		}

		return value;
	}
	catch (const std::invalid_argument& e)
	{
		LOG_MISC("Error when converting the string to uint32_t: {} | Input string: '{}'", e.what(), str);
		return 0;
	}
}

std::uint16_t Util::ConvertStringToUint16(const std::string& str)
{
	try
	{
		std::istringstream iss(str);
		std::uint16_t value;
		if (!(iss >> value))
			throw std::invalid_argument("Invalid number.");

		if (!iss.eof())
			throw std::invalid_argument("Invalid number.");

		return value;
	}
	catch (const std::invalid_argument& e)
	{
		LOG_MISC("Error when converting the string: {} for string: {}", e.what(), str);
		return 0;
	}
}

std::uint32_t Util::ConvertVariantToUInt32(const QVariant& variant)
{
	if (variant.canConvert<qint32>())
	{
		qint32 value = variant.toInt();
		return static_cast<std::uint32_t>(value);
	}
	else
		return 0;
}

std::string Util::ReverseString(const std::string& str)
{
	std::string reverseString;
	for (auto it = str.rbegin(); it != str.rend(); ++it)
		reverseString.push_back(*it);

	return reverseString;
}

std::string Util::ReverseStringParts(const std::string& str)
{
	std::string reversedStr;
	std::string currentPart;
	for (char c : str)
	{
		if (c == ' ')
		{
			reversedStr += ReverseString(currentPart) + ' ';
			currentPart = "";
		}
		else
		{
			currentPart += c;
		}
	}

	if (!currentPart.empty())
	{
		reversedStr += ReverseString(currentPart);
	}
	return reversedStr;
}

std::string Util::GetAccessRightStringName(AccessRights e)
{
	std::unique_ptr<TranslateText> _translate = std::make_unique<TranslateText>();
	switch (e)
	{
		
		case AccessRights::ACCESS_RIGHT_NONE: return _translate->TranslateString("translationAccess", "None").toStdString();
		case AccessRights::ACCESS_RIGHT_NORMAL: return _translate->TranslateString("translationAccess", "Normal").toStdString();
		case AccessRights::ACCESS_RIGHT_NORMAL_PLUS: return _translate->TranslateString("translationAccess", "Normal Plus").toStdString();
		case AccessRights::ACCESS_RIGHT_WAREHOUSEMAN: return _translate->TranslateString("translationAccess", "Warehouseman").toStdString();
		case AccessRights::ACCESS_RIGHT_USERMANAGER: return _translate->TranslateString("translationAccess", "Usermanager").toStdString();
		case AccessRights::ACCESS_RIGHT_ADMIN: return _translate->TranslateString("translationAccess", "Admin").toStdString();
		default: return _translate->TranslateString("translationAccess", "no Data").toStdString();
	}
}

std::string Util::ConvertUint32ToString(const std::uint32_t& value)
{
	try
	{
		std::string result = std::to_string(value);
		return result;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error converting uint32_t to string: " << e.what() << std::endl;
		return "";
	}
}

std::vector<std::uint16_t> Util::splitStringToVectorUint16(std::string& str, std::string delim)
{
	std::vector<std::uint16_t> tokens;
	std::size_t start = 0, end = 0;

	while ((end = str.find_first_of(delim, start)) != std::string::npos) {
		if (start != end)
		{
			tokens.push_back(ConvertStringToUint16(str.substr(start, end - start)));
		}
		start = end + 1;
	}
	if (start != str.length())
	{
		tokens.push_back(ConvertStringToUint16(str.substr(start)));
	}
	return tokens;
}

std::string Util::TrimSpacesFromString(std::string& str)
{
	auto start = str.find_first_not_of(' ');
	auto end = str.find_last_not_of(' ');

	return (start == std::string::npos || end == std::string::npos) ? "" : str.substr(start, end - start + 1);
}

std::string Util::RemoveLineBreaks(std::string& str)
{
	std::string result = str;
	result.erase(std::remove_if(result.begin(), result.end(), [](unsigned char c) {
		return c == '\n' || c == '\r';
		}), result.end());
	return result;
}

void Util::RemoveSpaces(std::string& str)
{
	// Remove all spaces from the string
	std::erase_if(str, [](unsigned char c) { return std::isspace(c); });
}

int Util::ConvertSizeToInt(size_t size)
{
	// Check if the size exceeds the maximum value an int can hold
	if (size > static_cast<size_t>((std::numeric_limits<int>::max)()))
		throw std::overflow_error("size exceeds int maximum value");

	// Safe to cast size to int and return
	return static_cast<int>(size);
}

void Util::onPushIncreaseOrDecrease(std::string operant, int orderCount, QSpinBox& spinbox)
{
	if (operant == "+")
	{
		int currentValue = spinbox.value();
		if (orderCount > currentValue)
			spinbox.setValue(currentValue + 1);
		else if (orderCount < spinbox.value())
			spinbox.setValue(orderCount);

	}
	else if (operant == "-")
	{
		int currentValue = spinbox.value();
		if (currentValue != 0)
			spinbox.setValue(currentValue - 1);
		else if (orderCount < spinbox.value())
			spinbox.setValue(orderCount);
	}
	else
	{
		LOG_MISC("OrderManager::onPushIncreaseOrDecrease: Error while increase or decrease spinbox");
	}
}

void Util::onPushIncreaseOrDecrease(std::string operant, int orderCount, QLabel& label)
{
	int currentValue = label.text().toInt();
	if (operant == "+")
	{
		if (orderCount > currentValue)
			label.setText(QString::number(currentValue + 1));
		else if (orderCount < currentValue)
			label.setText(QString::number(orderCount));
	}
	else if (operant == "-")
	{
		if (currentValue != 0)
			label.setText(QString::number(currentValue - 1));
		else if (orderCount < currentValue)
			label.setText(QString::number(orderCount));
	}
	else
	{
		LOG_MISC("OrderManager::onPushIncreaseOrDecrease: Error while increase or decrease label");
	}
}

void Util::CreateAndConnectAction(Qt::KeyboardModifiers modifier, int key, QObject* parent, std::function<void()> slotFunction, bool create)
{
	QKeySequence shortcut(modifier | key);
	QAction* existingAction = nullptr;

	// Check if the action with the given shortcut already exists
	QWidget* parentWidget = qobject_cast<QWidget*>(parent);
	if (parentWidget)
	{
		const QList<QAction*> actions = parentWidget->actions();
		for (QAction* action : actions)
		{
			if (action->shortcut() == shortcut)
			{
				existingAction = action;
				break;
			}
		}
	}

	if (create)
	{
		if (!existingAction)
		{
			QAction* newAction = new QAction(parent);
			newAction->setShortcut(shortcut);
			QObject::connect(newAction, &QAction::triggered, parent, [slotFunction]() {
				slotFunction();
				});
			if (parentWidget)
			{
				parentWidget->addAction(newAction);
			}
		}
	}
	else
	{
		if (existingAction)
		{
			QObject::disconnect(existingAction, &QAction::triggered, parent, nullptr);
			if (parentWidget)
			{
				parentWidget->removeAction(existingAction);
			}
			delete existingAction;
		}
	}
}

BarcodeType Util::identifyBarcodeType(const std::string& barcode)
{
	std::regex article_regex(R"(^[A-Z]{2}-.*$)");

	if (barcode.rfind("BRS-", 0) == 0)
		return BarcodeType::BARCODE_RETURN_SLIP;
	else if (barcode.rfind("BCU-", 0) == 0)
		return BarcodeType::BARCODE_COST_UNIT;
	else if (barcode.rfind("BMO-", 0) == 0)
		return BarcodeType::BARCODE_MO;
	else if (barcode.rfind("TTS-BG", 0) == 0)
		return BarcodeType::BARCODE_CONTROL_TTS;
	else
		return BarcodeType::BARCODE_ARTICLE; // Default -> Barcode is an ArticleBarcode
}

std::chrono::system_clock::time_point Util::ConvertUnixToSystemTimePoint(const std::uint64_t unixSeconds)
{
    return std::chrono::system_clock::time_point{std::chrono::seconds(unixSeconds)};
}

std::uint64_t Util::ConvertSystemTimePointToUnix(const std::chrono::system_clock::time_point& timePoint)
{
    return std::chrono::duration_cast<std::chrono::seconds>(timePoint.time_since_epoch()).count();
}

std::string Util::FormatTimePoint(std::chrono::system_clock::time_point timePoint, const std::string& format)
{
    std::time_t timeT = std::chrono::system_clock::to_time_t(timePoint);
    std::tm tmStruct{};
#if defined(_WIN32)
    localtime_s(&tmStruct, &timeT);
#else
    localtime_r(&timeT, &tmStruct);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tmStruct, format.c_str());
    return oss.str();
}

std::string Util::FormatTimePoint(std::chrono::system_clock::time_point timePoint, const char* format)
{
    return FormatTimePoint(timePoint, std::string(format));
}

std::string Util::FormatTimePoint(std::chrono::system_clock::time_point timePoint, const QString& format)
{
    return FormatTimePoint(timePoint, format.toStdString());
}

QDateTime Util::ConvertToQDateTime(std::chrono::system_clock::time_point timePoint)
{
    const std::time_t timeT = std::chrono::system_clock::to_time_t(timePoint);
    return QDateTime::fromSecsSinceEpoch(timeT);
}

std::chrono::system_clock::time_point Util::ConvertFromQDateTime(const QDateTime& dateTime)
{
    return std::chrono::system_clock::time_point{std::chrono::seconds(dateTime.toSecsSinceEpoch())};
}

std::string Util::FormatDate(const SystemTimePoint timePoint)
{
    constexpr auto format = "%d.%m.%Y";

    if (timePoint.time_since_epoch().count() == 0)
        return "";

    constexpr std::time_t minValidUnixTime = 1714514400;  // z. B. 01.05.2024 00:00 UTC

    if (std::chrono::system_clock::to_time_t(timePoint) <= minValidUnixTime)
        return "";

    return FormatTimePoint(timePoint, format);
}

std::chrono::system_clock::time_point Util::GetCurrentSystemPointTime() { return std::chrono::system_clock::now(); }

std::uint64_t Util::GetCurrentUnixTime()
{
    return std::chrono::duration_cast<std::chrono::seconds>(GetCurrentSystemPointTime().time_since_epoch()).count();
}

// Checks whether the input string is valid UTF-8.
bool Util::IsValidUtf8(const std::string& str)
{
	int expected = 0;

	for (unsigned char c : str)
	{
		if (expected == 0)
		{
			if ((c >> 5) == 0b110) expected = 1;
			else if ((c >> 4) == 0b1110) expected = 2;
			else if ((c >> 3) == 0b11110) expected = 3;
			else if ((c >> 7)) return false; // Invalid ASCII
		}
		else
		{
			if ((c >> 6) != 0b10) return false;
			--expected;
		}
	}

	return expected == 0;
}

// Converts a Latin-1 (Windows-1252) encoded string to UTF-8.
std::string Util::ConvertLatin1ToUtf8(const std::string& latin1)
{
	// Latin-1 -> UTF-16
	int wideLen = MultiByteToWideChar(1252, 0, latin1.c_str(), -1, nullptr, 0);
	if (wideLen <= 0) return "";

	std::wstring wideStr(wideLen, L'\0');
	MultiByteToWideChar(1252, 0, latin1.c_str(), -1, wideStr.data(), wideLen);

	// UTF-16 -> UTF-8
	int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	if (utf8Len <= 0) return "";

	std::string utf8Str(utf8Len, '\0');
	WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, utf8Str.data(), utf8Len, nullptr, nullptr);

	// Remove trailing null terminator
	if (!utf8Str.empty() && utf8Str.back() == '\0')
		utf8Str.pop_back();

	return utf8Str;
}

// Ensures the string is valid UTF-8. If not, tries to convert from Latin-1.
std::string Util::EnsureUtf8(const std::string& str)
{
	if (IsValidUtf8(str))
		return str;

	return ConvertLatin1ToUtf8(str);
}

std::string Util::ToUtf8String(const std::u8string& str)
{
	return std::string(str.begin(), str.end());
}

/************************************************************************************|
 ************************************************************************************|
 ********************************* Threading ****************************************|
 ************************************************************************************|
 ************************************************************************************/

void Util::RunInThread(std::function<void()> task, QObject* parent /*= nullptr*/)
{
	const auto workerThread = new QThread(parent);
	const auto worker = new ThreadWorker(std::move(task));

	worker->moveToThread(workerThread);

	// Standard signals & slots
	QObject::connect(workerThread, &QThread::started, worker, &ThreadWorker::runTask);
	QObject::connect(worker, &ThreadWorker::finished, workerThread, &QThread::quit);
	QObject::connect(worker, &ThreadWorker::finished, worker, &QObject::deleteLater);
	QObject::connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);

	workerThread->start();
}

QString Util::FormatEuro(const double value)
{
	// Format: 1.234.567,89 €
	return QLocale(QLocale::German).toString(value, 'f', 2) + " €";
}

/************************************************************************************|
 ************************************************************************************|
 ********************************* End Threading ************************************|
 ************************************************************************************|
 ************************************************************************************/

bool Util::ContainsOnlyDigists(const std::string& str)
{
	return std::ranges::all_of(str, ::isdigit);
}

void Util::TrimRight(std::string& str)
{
	str.erase(std::find_if(str.rbegin(), str.rend(),
		[](unsigned char ch) { return !std::isspace(ch); }).base(), str.end());
}

bool Util::IsPrefix(const std::string& prefix, const std::string& fullString)
{
	return fullString.find(prefix) == 0;  // Check if prefix is found at the beginning
}

std::pair<std::uint32_t, std::string> Util::ParseCostUnitString(const std::string& str)
{
	size_t delimiterPos = str.find(" - ");
	std::uint32_t costUnitID = std::stoul(str.substr(0, delimiterPos));  // ID extract
	std::string costUnitName = str.substr(delimiterPos + 3);  // Name extract
	return { costUnitID, costUnitName };
}

std::vector<std::string> Util::SplitStringBySpaces(const std::string& str)
{
	std::vector<std::string> result;
	std::istringstream stream(str);
	std::string word;

	// Split the string by spaces
	while (stream >> word)
		result.push_back(word);

	return result;
}

std::string Util::ConvertToUpperCase(const std::string& input)
{
	std::string result = input;

	for (char& c : result)
		c = std::toupper(c);

	return result;
}

std::string Util::ConvertToLowerCase(const std::string& input)
{
	std::string result = input;
	std::transform(result.begin(), result.end(), result.begin(),
		[](unsigned char c) { return std::tolower(c); });
	return result;
}

QLocale Util::GetLocaleFromLanguage(const std::string& languageCode)
{
	if (languageCode == "en_GB") return QLocale(QLocale::English, QLocale::UnitedKingdom);
	if (languageCode == "ja_JP") return QLocale(QLocale::Japanese, QLocale::Japan);
	if (languageCode == "pl_PL") return QLocale(QLocale::Polish, QLocale::Poland);
	if (languageCode == "ru_RU") return QLocale(QLocale::Russian, QLocale::Russia);

	// Default: German
	return QLocale(QLocale::German, QLocale::Germany);
}

QString Util::GetTTSModeString(TextToSpeechMode mode)
{
	std::unique_ptr<TranslateText> _translate = std::make_unique<TranslateText>();
	switch (mode)
	{
	case TextToSpeechMode::NONE:
		break;
	case TextToSpeechMode::ARTICLEPOSITION_BOXNAME_AND_COUNT:
	{
		return _translate->TranslateString("translationTTS", "Position, Boxname and Count");
		break;
	}
	case TextToSpeechMode::ARTICLENAME:
	{
		return _translate->TranslateString("translationTTS", "Articlename");
		break;
	}
	case TextToSpeechMode::ARTICLENUMBER:
	{
		return _translate->TranslateString("translationTTS", "Articlenumber");
		break;
	}
	case TextToSpeechMode::ARTICLENUMBER_AND_ARTICLENAME:
	{
		return _translate->TranslateString("translationTTS", "Articlenumber and Name");
		break;
	}
	default:
		break;
	}
}

void Util::SetLoading(QPushButton* button, bool enable, std::initializer_list<QWidget*> otherWidgets)
{
	if (!button)
		return;

	if (enable)
	{
		// Prevent duplicate activation
		if (button->property("loadingActive").toBool())
			return;

		// Store current text and icon
		button->setProperty("originalText", button->text());
		button->setProperty("originalIcon", button->icon());
		button->setProperty("loadingActive", true);

		button->setText("");
		button->setIcon(QIcon());

		// Create loading animation
		QMovie* movie = new QMovie(":/Loading/resources/loading/icons8_loading_normal.gif", QByteArray(), button);
		movie->setScaledSize(QSize(30, 30));

		QLabel* label = new QLabel(button);
		label->setMovie(movie);
		label->setAlignment(Qt::AlignCenter);
		movie->start();

		// Add layout if not present
		if (!button->layout())
		{
			auto* layout = new QHBoxLayout(button);
			layout->setContentsMargins(0, 0, 0, 0);
			button->setLayout(layout);
		}

		button->layout()->addWidget(label);

		// Store movie and label pointers
		button->setProperty("loadingMovie", QVariant::fromValue(static_cast<void*>(movie)));
		button->setProperty("loadingLabel", QVariant::fromValue(static_cast<void*>(label)));

		// Disable additional widgets
		auto* widgetList = new std::vector<QPointer<QWidget>>();
		widgetList->reserve(otherWidgets.size());

		for (QWidget* w : otherWidgets)
		{
			if (w)
			{
				w->setEnabled(false);
				widgetList->push_back(QPointer<QWidget>(w));
			}
		}

		// Store pointer list
		button->setProperty("disabledWidgets", QVariant::fromValue(static_cast<void*>(widgetList)));
	}
	else
	{
		// Only handle if active
		if (!button->property("loadingActive").toBool())
			return;

		// Restore original text and icon
		button->setText(button->property("originalText").toString());
		button->setIcon(button->property("originalIcon").value<QIcon>());

		// Remove loading label
		if (auto* label = static_cast<QLabel*>(button->property("loadingLabel").value<void*>()))
			label->deleteLater();

		// Stop and delete animation
		if (auto* movie = static_cast<QMovie*>(button->property("loadingMovie").value<void*>()))
		{
			movie->stop();
			movie->deleteLater();
		}

		// Re-enable stored widgets
		if (auto* stored = static_cast<std::vector<QPointer<QWidget>>*>(button->property("disabledWidgets").value<void*>()))
		{
			for (const QPointer<QWidget>& w : *stored)
			{
				if (w)
					w->setEnabled(true);
			}
			delete stored;
		}

		// Clear properties
		button->setProperty("loadingActive", false);
		button->setProperty("originalText", {});
		button->setProperty("originalIcon", {});
		button->setProperty("loadingMovie", {});
		button->setProperty("loadingLabel", {});
		button->setProperty("disabledWidgets", {});
	}
}


std::vector<char> Util::ReadBlobOrEmpty(sql::ResultSet* rs, int col)
{
	if (!rs || rs->isNull(col))
		return {};

	if (auto* s = rs->getBlob(col))
		return { std::istreambuf_iterator<char>(*s), std::istreambuf_iterator<char>() };

	return {};
}

void Util::SetLoading(QPushButton* button, bool enable)
{
	SetLoading(button, enable, {});
}

/* AMS Helper */

std::optional<uint64_t> Util::GetSelectedCallerId(const QTableView* table)
{
    if (!table)
        return std::nullopt;

    QModelIndex current = table->currentIndex();
    if (!current.isValid())
        return std::nullopt;

    return GetCallerIdFromIndex(table, current);
}

std::optional<std::uint64_t> Util::GetCallerIdFromIndex(const QTableView* table, const QModelIndex& viewIndex)
{
    if (!table || !viewIndex.isValid())
        return std::nullopt;

    QAbstractItemModel* model = table->model();
    if (!model)
        return std::nullopt;

    QModelIndex sourceIndex = viewIndex;

    // Map through proxy if needed
    if (auto* proxy = qobject_cast<QSortFilterProxyModel*>(model); proxy)
    {
        sourceIndex = proxy->mapToSource(viewIndex);
        model = proxy->sourceModel();
    }

    if (!model || !sourceIndex.isValid())
        return std::nullopt;

    QVariant idVar = sourceIndex.data(Qt::UserRole);

    if (!idVar.isValid())
    {
        // Fallback: assume ID is stored in column 0 (maybe hidden in view).
        QModelIndex idIndex = model->index(sourceIndex.row(), 0);
        if (!idIndex.isValid())
            return std::nullopt;

        idVar = idIndex.data(Qt::DisplayRole);
    }

    bool ok = false;
    const auto value = idVar.toULongLong(&ok);
    if (!ok)
        return std::nullopt;

    return static_cast<std::uint64_t>(value);
}

std::optional<std::uint64_t> Util::GetIdFromIndex(const QTableView* table, const QModelIndex& viewIndex, int idColumn /*= 0*/, int idRole /*= Qt::UserRole*/)
{
    if (!table || !viewIndex.isValid())
        return std::nullopt;

    QAbstractItemModel* model = table->model();
    if (!model)
        return std::nullopt;

    QModelIndex sourceIndex = viewIndex;

    // Map through proxy if needed
    if (auto* proxy = qobject_cast<QSortFilterProxyModel*>(model); proxy)
    {
        sourceIndex = proxy->mapToSource(viewIndex);
        model = proxy->sourceModel();
    }

    if (!model || !sourceIndex.isValid())
        return std::nullopt;

    QVariant idVar;

    // First: try the given role on the source index (default: Qt::UserRole)
    if (idRole != Qt::DisplayRole)
    {
        idVar = sourceIndex.data(idRole);
    }

    // Fallback: use the given column with DisplayRole (if enabled)
    if ((!idVar.isValid() || idVar.isNull()) && idColumn >= 0)
    {
        QModelIndex idIndex = model->index(sourceIndex.row(), idColumn);
        if (!idIndex.isValid())
            return std::nullopt;

        idVar = idIndex.data(Qt::DisplayRole);
    }

    if (!idVar.isValid() || idVar.isNull())
        return std::nullopt;

    bool ok = false;
    const auto value = idVar.toULongLong(&ok);
    if (!ok)
        return std::nullopt;

    return static_cast<std::uint64_t>(value);
}

std::optional<std::uint64_t> Util::GetIdFromSelection(const QTableView* table, int idColumn /*= 0*/, int idRole /*= Qt::UserRole*/)
{
    if (!table)
        return std::nullopt;

    const QModelIndex current = table->currentIndex();
    if (!current.isValid())
        return std::nullopt;

    return GetIdFromIndex(table, current, idColumn, idRole);
}

QIcon Util::QlementineIconsMaker(Icons16 id, const QSize& size /*= { 16, 16 }*/)
{
    const auto svgPath = oclero::qlementine::icons::iconPath(id);
    if (auto* style = oclero::qlementine::appStyle())
    {
        return style->makeThemedIcon(svgPath, size);
    }
    else
    {
        return QIcon(svgPath);
    }
}

QString Util::FormatOpenDurationQString(const SystemTimePoint& createdAt, const SystemTimePoint& closedAt, TicketStatus status)
{
    // invalid timestamp -> empty
    if (createdAt.time_since_epoch().count() == 0)
        return {};

    SystemTimePoint endTime;

    if ((status == TicketStatus::TICKET_STATUS_RESOLVED || status == TicketStatus::TICKET_STATUS_CLOSED) &&
        closedAt.time_since_epoch().count() != 0)
    {
        endTime = closedAt;
    }
    else
    {
        endTime = Util::GetCurrentSystemPointTime();
    }

    if (endTime <= createdAt)
        return TranslateText::translateNew("Duration", "0 minutes");

    auto diffSec = std::chrono::duration_cast<std::chrono::seconds>(endTime - createdAt).count();

    const auto secPerDay = 24 * 60 * 60;
    const auto secPerHour = 60 * 60;
    const auto secPerMin = 60;

    const auto days = diffSec / secPerDay;
    diffSec %= secPerDay;

    const auto hours = diffSec / secPerHour;
    diffSec %= secPerHour;

    const auto minutes = diffSec / secPerMin;

    QString result;

    const QString txtDay = TranslateText::translateNew("Duration", "day");
    const QString txtDays = TranslateText::translateNew("Duration", "days");
    const QString txtHour = TranslateText::translateNew("Duration", "hour");
    const QString txtHours = TranslateText::translateNew("Duration", "hours");
    const QString txtMinute = TranslateText::translateNew("Duration", "minute");
    const QString txtMinutes = TranslateText::translateNew("Duration", "minutes");

    if (days > 0)
        result += QString("%1 %2 ").arg(days).arg(days == 1 ? txtDay : txtDays);

    if (hours > 0)
        result += QString("%1 %2 ").arg(hours).arg(hours == 1 ? txtHour : txtHours);

    result += QString("%1 %2").arg(minutes).arg(minutes == 1 ? txtMinute : txtMinutes);

    return result.trimmed();
}

QIcon Util::QlementineIconsMaker(Icons16 id, const QSize& size /*= {16, 16}*/, const std::optional<QColor>& color /*= std::nullopt*/)
{
    const auto svgPath = oclero::qlementine::icons::iconPath(id);

    // No color override: use themed icon (current behavior)
    if (!color.has_value() || !color->isValid())
    {
        if (auto* style = oclero::qlementine::appStyle())
        {
            return style->makeThemedIcon(svgPath, size);
        }

        return QIcon(svgPath);
    }

    // Base icon (prefer Qlementine style if available)
    QIcon baseIcon;
    if (auto* style = oclero::qlementine::appStyle())
    {
        baseIcon = style->makeThemedIcon(svgPath, size);
    }
    else
    {
        baseIcon = QIcon(svgPath);
    }

    // Determine device pixel ratio for HiDPI
    qreal dpr = 1.0;
    if (const auto* screen = QGuiApplication::primaryScreen())
    {
        dpr = screen->devicePixelRatio();
    }

    const QSize deviceSize = size * dpr;

    QPixmap src = baseIcon.pixmap(deviceSize);
    src.setDevicePixelRatio(dpr);

    if (src.isNull())
    {
        // Fallback if anything went wrong
        return baseIcon;
    }

    QPixmap dst(src.size());
    dst.setDevicePixelRatio(dpr);
    dst.fill(Qt::transparent);

    // Tint icon using alpha as mask
    {
        QPainter painter(&dst);

        // First, fill with the desired color
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.fillRect(dst.rect(), *color);

        // Then, apply the original alpha as mask
        painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        painter.drawPixmap(0, 0, src);
    }

    return QIcon(dst);
}

QString Util::FormatDateTimeQString(const SystemTimePoint& tp, const QString& format /*= "dd.MM.yyyy HH:mm:ss"*/)
{
    // zero / invalid timestamp -> empty string
    if (tp.time_since_epoch().count() == 0)
        return {};

    // convert to QDateTime
    QDateTime dt = Util::ConvertToQDateTime(tp);

    if (!dt.isValid())
        return {};

    return dt.toString(format);
}

QString Util::CurrentDateTimeString()
{
    // Format: "YYYY-MM-DD HH:MM:SS"
    return QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
}

std::string Util::CurrentDateTimeStringStd()
{
    return CurrentDateTimeString().toStdString();
}

QString Util::FormatDateTime(const SystemTimePoint& tp)
{
    if (tp.time_since_epoch().count() == 0)
        return {};

    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();

    return QDateTime::fromMSecsSinceEpoch(ms).toString("yyyy-MM-dd HH:mm:ss");
}

std::string Util::FormatDateTimeStd(const SystemTimePoint& tp)
{
    return FormatDateTime(tp).toStdString();
}

QString Util::BuildWelcomeText()
{
    std::unique_ptr<TranslateText> translator = std::make_unique<TranslateText>();

    QTime now = QTime::currentTime();
    int hour = now.hour();

    QStringList greetings;

    if (hour >= 5 && hour < 11)  // morning
    {
        greetings << translator->TranslateString("translationWelcome", "Good morning")
                  << translator->TranslateString("translationWelcome", "Morning")
                  << translator->TranslateString("translationWelcome", "Nice morning");
    }
    else if (hour >= 11 && hour < 15)  // noon / midday
    {
        greetings << translator->TranslateString("translationWelcome", "Good afternoon")
                  << translator->TranslateString("translationWelcome", "Nice afternoon");
    }
    else if (hour >= 15 && hour < 21)  // evening
    {
        greetings << translator->TranslateString("translationWelcome", "Good evening")
                  << translator->TranslateString("translationWelcome", "Nice evening");
    }
    else  // night
    {
        greetings << translator->TranslateString("translationWelcome", "Good night")
                  << translator->TranslateString("translationWelcome", "Good Night shift");
    }

    int index = QRandomGenerator::global()->bounded(greetings.size());
    return greetings.at(index);
}

std::int64_t Util::OpenMinutes(const SystemTimePoint& createdAt, const SystemTimePoint& closedAt, TicketStatus status)
{
    if (createdAt.time_since_epoch().count() == 0)
        return 0;

    SystemTimePoint endTime;

    if ((status == TicketStatus::TICKET_STATUS_RESOLVED || status == TicketStatus::TICKET_STATUS_CLOSED) && closedAt.time_since_epoch().count() != 0)
    {
        endTime = closedAt;
    }
    else
    {
        endTime = Util::GetCurrentSystemPointTime();
    }

    if (endTime <= createdAt)
        return 0;

    return std::chrono::duration_cast<std::chrono::minutes>(endTime - createdAt).count();
}

std::optional<QColor> Util::DurationColor(std::int64_t minutesOpen)
{
    if (minutesOpen >= 48 * 60)
        return QColor(220, 80, 80, 120);  // red

    if (minutesOpen >= 12 * 60)
        return QColor(255, 170, 60, 110);  // orange

    if (minutesOpen >= 4 * 60)
        return QColor(255, 215, 80, 100);  // yellow

    return QColor(90, 200, 120, 90);  // green
}

void Util::SetupStretchColumn(QTableView* tableView, int stretchColumn, int minStretchWidth)
{
    if (tableView == nullptr)
    {
        return;
    }

    QAbstractItemModel* model = tableView->model();
    if (model == nullptr)
    {
        return;
    }

    QHeaderView* header = tableView->horizontalHeader();
    const int columnCount = model->columnCount();

    if (stretchColumn < 0 || stretchColumn >= columnCount)
    {
        return;
    }

    header->setStretchLastSection(false);
    tableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    for (int col = 0; col < columnCount; ++col)
    {
        if (col == stretchColumn)
        {
            header->setSectionResizeMode(col, QHeaderView::Stretch);
        }
        else
        {
            header->setSectionResizeMode(col, QHeaderView::ResizeToContents);
        }
    }

    if (minStretchWidth > 0)
    {
        const int current = header->sectionSize(stretchColumn);
        header->resizeSection(stretchColumn, std::max(minStretchWidth, current));
    }
}

QStringList Util::TokenizeSearch(const QString& input)
{
    const QString trimmed = input.trimmed();
    if (trimmed.isEmpty())
        return {};

    return trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
}
