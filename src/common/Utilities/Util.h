/*
Copyright 2023 - 2024 by Severin
and TrinityCore
*/

#pragma once
#include <qspinbox.h>

#include <QAction>
#include <QKeySequence>
#include <QLabel>
#include <QMainWindow>
#include <QObject>
#include <QPushButton>
#include <QString>
#include <format>
#include <oclero/qlementine/icons/Icons12.hpp>
#include <oclero/qlementine/icons/Icons16.hpp>
#include <oclero/qlementine/icons/Icons32.hpp>
#include <sstream>
#include <string>
#include <vector>

#include "Duration.h"
#include "ThreadWorker.h"

class TranslationHandler;
enum class ErrorCodes : std::uint64_t;
enum class SuccessfullCodes : std::uint64_t;
enum class AccessRights : std::uint8_t;
enum class BarcodeType : std::uint8_t;
enum class TextToSpeechMode : std::uint8_t;
enum class TicketStatus : std::uint8_t;

class Tokenizer
{
public:
	typedef std::vector<char const*> StorageType;

	typedef StorageType::size_type size_type;

	typedef StorageType::const_iterator const_iterator;
	typedef StorageType::reference reference;
	typedef StorageType::const_reference const_reference;

public:
	Tokenizer(const std::string& src, char const sep, std::uint32_t vectorReserve = 0, bool keepEmptyStrings = true);
	~Tokenizer() { delete[] m_str; }

	const_iterator begin() const { return m_storage.begin(); }
	const_iterator end() const { return m_storage.end(); }

	size_type size() const { return m_storage.size(); }

	reference operator [] (size_type i) { return m_storage[i]; }
	const_reference operator [] (size_type i) const { return m_storage[i]; }


private:
	char* m_str;
	StorageType m_storage;
};

namespace Util
{
	bool QStringContainsOnlyDigits(const QString& string);
	std::uint8_t ConvertStringToUint8(const std::string& str);
	std::uint32_t ConvertStringToUint32(const std::string& str);
	std::uint16_t ConvertStringToUint16(const std::string& str);
	std::uint32_t ConvertVariantToUInt32(const QVariant& variant);
	std::string ReverseString(const std::string& str);
	std::string ReverseStringParts(const std::string& str);
	std::string GetAccessRightStringName(AccessRights e);
	std::string ConvertUint32ToString(const std::uint32_t& value);
	std::vector<std::uint16_t> splitStringToVectorUint16(std::string& str, std::string delim);
	std::string TrimSpacesFromString(std::string& str);
	std::string RemoveLineBreaks(std::string& str);
	void RemoveSpaces(std::string& str);
	std::string ConvertToUpperCase(const std::string& input);
	std::string ConvertToLowerCase(const std::string& input);

	bool ContainsOnlyDigists(const std::string& str);

	void TrimRight(std::string& str);

	// Helper function to check if one string is a prefix of another
	bool IsPrefix(const std::string& prefix, const std::string& fullString);

	std::pair<std::uint32_t, std::string> ParseCostUnitString(const std::string& str);

	std::vector<std::string> SplitStringBySpaces(const std::string& str);

	int ConvertSizeToInt(size_t size);

	void onPushIncreaseOrDecrease(std::string operant, int orderCount, QSpinBox& spinbox);
	void onPushIncreaseOrDecrease(std::string operant, int orderCount, QLabel& label);
	void CreateAndConnectAction(Qt::KeyboardModifiers modifier, int key, QObject* parent, std::function<void()> slotFunction, bool create);

	BarcodeType identifyBarcodeType(const std::string& barcode);

	// Can be removed if we can use c++23 for 'std::ranges::join_with'
    std::string join(const std::vector<std::string>& elements, const std::string& delimiter);

	// Time Handling
    [[nodiscard]] std::chrono::system_clock::time_point ConvertUnixToSystemTimePoint(std::uint64_t unixSeconds);
    [[nodiscard]] std::uint64_t ConvertSystemTimePointToUnix(const std::chrono::system_clock::time_point& timePoint);
    [[nodiscard]] std::string FormatTimePoint(std::chrono::system_clock::time_point timePoint, const std::string& format);
    [[nodiscard]] std::string FormatTimePoint(std::chrono::system_clock::time_point timePoint, const char* format);
    [[nodiscard]] std::string FormatTimePoint(std::chrono::system_clock::time_point timePoint, const QString& format);
    [[nodiscard]] QDateTime ConvertToQDateTime(std::chrono::system_clock::time_point timePoint);
    [[nodiscard]] std::chrono::system_clock::time_point ConvertFromQDateTime(const QDateTime& dateTime);
    [[nodiscard]] std::string FormatDate(SystemTimePoint timePoint);
    [[nodiscard]] std::chrono::system_clock::time_point GetCurrentSystemPointTime();
    [[nodiscard]] std::uint64_t GetCurrentUnixTime();
    QString FormatDateTimeQString(const SystemTimePoint& tp, const QString& format = "dd.MM.yyyy HH:mm:ss");
    QString CurrentDateTimeString();
    std::string CurrentDateTimeStringStd();
    QString FormatDateTime(const SystemTimePoint& tp);
    std::string FormatDateTimeStd(const SystemTimePoint& tp);


	template<typename... Args>
	std::string FormatMessageUtil(std::string_view message, Args&&... args)
	{
		return std::vformat(message, std::make_format_args(args...));
	}

	// UTF8 Check & Fix
	bool IsValidUtf8(const std::string& str);
	std::string ConvertLatin1ToUtf8(const std::string& latin1);
	std::string EnsureUtf8(const std::string& str);
	std::string ToUtf8String(const std::u8string& str);

	// Threading
	void RunInThread(std::function<void()> task, QObject* parent = nullptr);
	// Money
	QString FormatEuro(double value);

	// Threading End

	QLocale GetLocaleFromLanguage(const std::string& languageCode);

	QString GetTTSModeString(TextToSpeechMode mode);

	/**
	* @brief Shows a loading animation on a button and disables other widgets.
	*
	* @param button Target button where loading animation is shown.
	* @param enable If true, show animation and disable; if false, restore state.
	* @param otherWidgets Optional widgets to disable during loading.
	*/
	void SetLoading(QPushButton* button, bool enable, std::initializer_list<QWidget*> otherWidgets);

	/**
	 * @brief Shows a loading animation on a button only.
	 *
	 * @param button Target button where loading animation is shown.
	 * @param enable If true, show animation; if false, restore original state.
	 */
	void SetLoading(QPushButton* button, bool enable);


	/* MySQL Helper */
	std::vector<char> ReadBlobOrEmpty(sql::ResultSet* rs, int col);


	/* AMS Helper */
	std::optional<uint64_t> GetSelectedCallerId(const QTableView* table);
    // Returns caller id using a given view index (e.g. from clicked/activated).
    std::optional<std::uint64_t> GetCallerIdFromIndex(const QTableView* table, const QModelIndex& viewIndex);

	std::optional<std::uint64_t> GetIdFromIndex(const QTableView* table, const QModelIndex& viewIndex, int idColumn = 0, int idRole = Qt::UserRole);
	std::optional<std::uint64_t> GetIdFromSelection(const QTableView* table, int idColumn = 0, int idRole = Qt::UserRole);

	/* QLementine Icons */
	using Icons16 = oclero::qlementine::icons::Icons16;
	QIcon QlementineIconsMaker(Icons16 id, const QSize& size = { 16, 16 });
    QIcon QlementineIconsMaker(Icons16 id, const QSize& size /*= {16, 16}*/, const std::optional<QColor>& color /*= std::nullopt*/);

	template <typename T>
    T ComboGetValue(QComboBox* box, int role = Qt::UserRole)
    {
        return box->currentData(role).value<T>();
    }

	QString FormatOpenDurationQString(const SystemTimePoint& createdAt, const SystemTimePoint& closedAt, TicketStatus status);
	QString BuildWelcomeText();
    std::int64_t OpenMinutes(const SystemTimePoint& createdAt, const SystemTimePoint& closedAt, TicketStatus status);
    std::optional<QColor> DurationColor(std::int64_t minutesOpen);

	void SetupStretchColumn(QTableView* tableView, int stretchColumn, int minStretchWidth);

    QStringList TokenizeSearch(const QString& input);

}
