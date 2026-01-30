#pragma once
#include <QSettings>
#include <QString>
#include <QSize>
#include <QPoint>
#include <memory>

#include "Crypto.h"
#include "Logger.h"
#include "SharedDefines.h"

struct TLSConfig
{
    bool enabled = false;
    std::string caFile;
    std::string certFile;
    std::string keyFile;
    std::string cipherList;
    bool verifyServerCert = true;
};

struct SSHConfig
{
    bool enabled = false;
    std::string host;
    std::uint16_t port = 22;
    std::string username;
    std::string password;
    std::string privateKeyFile;
    std::string passphrase;
};

struct PoolLimits
{
    std::size_t minSize = 1;
    std::size_t maxSize = 5;
    std::size_t maxQueueDepth = 1024;
};

struct ReplicaConfig
{
    bool enabled = false;
    std::string lagQuery = "SHOW SLAVE STATUS";
    std::uint32_t maxAllowedLagSeconds = 5;
};

struct DiagnosticsConfig
{
    bool enableSelfTest = true;
    std::uint32_t selfTestIntervalSeconds = 60;
};

struct MaintenanceConfig
{
    std::uint32_t pingIntervalSeconds = 30;
    std::uint32_t reconnectDelaySeconds = 5;
};

struct MySQLSettings
{
	std::string hostname;
	std::string username;
	std::string password;
	std::string port;
	std::string database;
	bool isActive;
    TLSConfig tls;
    SSHConfig ssh;
};

struct PoolConfig
{
    MySQLSettings primary;
    MySQLSettings replica;
    bool useReplicaForReads = false;
    PoolLimits syncLimits;
    PoolLimits asyncLimits;
    MaintenanceConfig maintenance;
    ReplicaConfig replicaConfig;
    DiagnosticsConfig diagnostics;
};

struct LabelLayoutSettings
{
	double widthMM;
	double heightMM;
	double fontSizePt;
};

struct SoundSettings
{
	int volume;
	bool enabled;
};

struct SerialSettingsNew
{
	std::string portName;
	QSerialPort::BaudRate baudRate;
	QSerialPort::DataBits dataBits;
	QSerialPort::Parity parity;
	QSerialPort::StopBits stopBits;
	QSerialPort::FlowControl flowControl;
};

struct SSHSettings
{
	std::string hostname;
	int port;
	std::string username;
	std::string programmPath;
	bool isActive;
	int localPort;
};

struct CrashRotateData
{
    bool isActive;
    std::uint32_t maxCrashDumps;
    std::uint32_t maxDays;
};

/*
struct WindowGeometry
{
	QSize size;
	QPoint position;
};*/

struct NetworkShareData
{
    QString networkShareUser {};
    QString networkSharePath {};
    QString networkSharePassword {};
    bool networkShareActive = false;
};

struct LocalFileData
{
    QString rootPath {};
    bool isActive = false;
};

enum class GlobalSettings : std::uint8_t
{
	GLOBAL_SETTING_ORDER_FILE_PATH		= 1,
	GLOBAL_SETTING_ORDER_FILE_FOLDER	= 2,
    GLOBAL_SETTING_AMS_LOCAL_FILE_ROOT  = 3,

	GLOBAL_SETTING_MAX,
};

enum class GlobalSettingsTypeLocalFile : std::uint8_t
{
    DATA_TYPE_ROOT_PATH     = 0,
    DATA_TYPE_IS_ACTIVE     = 1,
    DATA_TYPE_DEACTIVATED   = 2,
};


class SettingsManager
{
public:
	static SettingsManager& instance();

	void InitDefaults() const;
	void LoadSettings();

	// Getters
	MySQLSettings getMySQLSettings() { return _mySQLSettings; }
    MySQLSettings getAMSMySQLSettings() { return _amsSettings; }
	LabelLayoutSettings getLabelLayout() const { return _labelLayoutSettings; }
	SoundSettings getSoundSettings() const { return _soundSettings; }
	CompanyLocations getCompanyLocation() const { return _companyLocation; }
	SerialSettings getChipReader() const { return _chipReader; }
	SerialSettings getBarcodeReader() const { return _barcodeReader; }
	SSHSettings getSSHSettings() const { return _sshSettings; }
    CrashRotateData getCrashRotateData() const { return _crashRotateData; }
    NetworkShareData getNetworkShareData() const { return _networkShareData; }
    LocalFileData getLocalFileData() const { return _localFileData; }

	std::string getLanguage() { return _language; }
	std::string getLocalCommunicationPort() { return _localCommunicationPort; }
	std::string getLogPath() const { return _logPath; }
	std::string getLogPrefix() const { return _logPrefix; }
	std::string getOrderConfirmationFilePath() const { return _orderConfirmationFilePath; }
	std::string getLabelTemplateOverrideName() const { return _labelTemplateOverrideName; }

	std::uint8_t getStyleSelection() const { return _styleSelection; }
	std::string GetStyleString(Styles styles);
	std::string GetStyleString() const;

	bool isLogLevelEnabled(LogLevel level) const;
	bool isFullScreen() const { return _isFullScreen; }
	bool isPersonalStyleEnabled() const { return _personalStyleEnabled; }
	bool isPersonalLanguageEnabled() const { return _personalLanguageEnabled; }
	bool isVirtualKeyboardEnabled() const { return _virtualKeyboardEnabled; }
	bool isTTSActive() const { return _isTTSActive; }

    // File Handling
    bool isNetworkFileEnabled() const { return _networkShareData.networkShareActive; }
    bool isLocalFileEnabled() const { return _localFileData.isActive; }

	std::unordered_map<LogLevel, bool> GetLogEnabledDisabled() const { return _enabledLogLevels; }

	// End getters

	// Window geometry
	WindowGeometry getWindowGeometry(WindowNames name) const;
	void setWindowGeometry(WindowNames name, const WindowGeometry& geo) const;
	// End window geometry

	// Setters
	void SetMySQLData(const MySQLSettings& mySQLSetting) const;
    void SetAMSMySQLData(const MySQLSettings& amsMySQLSetting) const;
	void SetLabelLayout(const LabelLayoutSettings& labelLayout);
	void SetSoundSettings(const SoundSettings& soundSettings);
	void SetCompanyLocation(const CompanyLocations& companyLocation);
	void SetChipReader(const SerialSettings& chipReader) const;
	void SetBarcodeReader(const SerialSettings& barcodeReader) const;
	void SetLanguage(const std::string& language);
	void SetLogPath(const std::string& logPath) const;
	void SetLogEnabledDisabled(const std::unordered_map<LogLevel, bool>& enabledLogLevels);
	void SetLogPrefix(const std::string& logPrefix) const;
	void SetOrderConfirmationFilePath(const std::string& orderConfirmationFilePath) const;
	void SetStyleSelection(const std::uint8_t styleSelection) const;
	void setIsFullScreen(bool isActive = false) const;
	void SetPersonalStyleEnabled(bool isActive = false);
	void SetPersonalLanguageEnabled(bool isActive = false);
	void SetVirtualKeyboardEnabled(bool isActive = false) const;
	void SetLabelTemplateOverrideName(const std::string& labelTemplateOverrideName);
	void SetTTSActive(bool isActive = false) const;
    void SetCrashRotateData(const CrashRotateData& crashRotateData) const;
    void SetNetworkShareData(const NetworkShareData& networkData) const;
    void SetLocalFileData(const LocalFileData& localFileData) const;

	// Global Settings
	void LoadGlobalSettings();
	void LoadGlobalOrderFilePath();
	void LoadGlobalOrderFolder();

    void SynSettings();


private:
	void loadMySQLSettings();
	void loadLabelLayoutSettings();
	void loadSoundSettings();
	void loadLanguage();
	void loadFullScreen();
	void loadPersonalStyleEnabled();
	void loadPersonalLanguageEnabled();
	void loadVirtualKeyboardEnabled();
	void loadLocalCommunicationPort();
	void loadLogPath();
	void loadLogPrefix();
	void loadStyleSelection();
	void loadCompanyLocation();
	void loadChipReader();
	void loadBarcodeReader();
	void loadSSHSettings();
	void loadOrderConfirmationFilePath();
	void loadEnabledLogLevels();
	void loadWindowGeometry();
	void loadLabelTemplateOverrideName();
	void loadTTSActive();
    void loadCrashRotateData();
    void loadNetworkShareData();
    void loadLocalFileData();

	// Global Settings
	void HandleGlobalSetting(const GlobalSettings setting);
    void LoadGlobalLocalFileData();

    // only Network or local file can be active not both at the same time
    void checkFileHandling();

	// Check function
	// If the path does not exist or the directory cannot be created, return the root directory with the folder.
	std::string CheckPath(const std::string& inputPath, const std::string& lastFolderName) const;

	SettingsManager();
	std::unique_ptr<QSettings> settings;
	std::unique_ptr<Crypto> _crypto;

	// Cached settings
	MySQLSettings _mySQLSettings;
	MySQLSettings _amsSettings;
	LabelLayoutSettings _labelLayoutSettings;
	SoundSettings _soundSettings;
	std::string _language;
	bool _isFullScreen;
	bool _personalStyleEnabled;
	bool _personalLanguageEnabled;
	bool _virtualKeyboardEnabled;
	std::string _localCommunicationPort;
	std::string _logPath;
	std::string _logPrefix;
	std::uint8_t _styleSelection;
	std::string _labelTemplateOverrideName;
	CompanyLocations _companyLocation;
	SerialSettings _chipReader;
	SerialSettings _barcodeReader;
	SSHSettings _sshSettings;
	std::string _orderConfirmationFilePath;
	std::unordered_map<LogLevel, bool> _enabledLogLevels;
	std::unordered_map<WindowNames, WindowGeometry> _windowGeometry;
	bool _isTTSActive;
    CrashRotateData _crashRotateData;
    NetworkShareData _networkShareData;
    LocalFileData _localFileData;
};

inline SettingsManager& GetSettings()
{
	return SettingsManager::instance();
}
