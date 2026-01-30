#include "SettingsManager.h"

#include "BootstrapLogger.h"
#include "ConnectionGuard.h"
#include "NetworkSharePasswordStore.h"
#include "PasswordStore.h"

SettingsManager& SettingsManager::instance()
{
    static SettingsManager _instance;
    return _instance;
}

void SettingsManager::InitDefaults() const
{
    // MySQL
    settings->beginGroup("MySQL");
    settings->beginGroup("IMS");
    if (!settings->contains("Hostname"))
        settings->setValue("Hostname", "localhost");
    if (!settings->contains("Username"))
        settings->setValue("Username", "root");
    if (!settings->contains("Port"))
        settings->setValue("Port", "3306");
    if (!settings->contains("Database"))
        settings->setValue("Database", "ims_db");
    if (!settings->contains("IsActive"))
        settings->setValue("IsActive", true);
    settings->endGroup();
    settings->beginGroup("AMS");
    if (!settings->contains("Hostname"))
        settings->setValue("Hostname", "localhost");
    if (!settings->contains("Username"))
        settings->setValue("Username", "root");
    if (!settings->contains("Port"))
        settings->setValue("Port", "3306");
    if (!settings->contains("Database"))
        settings->setValue("Database", "ams_db");
    if (!settings->contains("IsActive"))
        settings->setValue("IsActive", false);
    settings->endGroup();
    settings->endGroup();

    /* Logging Data */
    settings->beginGroup("Logging");
    if (!settings->contains("LogPath"))
        settings->setValue("LogPath", "C:/ProgramData/AMS/logs");
    if (!settings->contains("EnabledLogLevel"))
        settings->setValue("EnabledLogLevel", "1,2,3,4,5,6");
    settings->endGroup();

    /* Label data */
    settings->beginGroup("LabelLayout");
    if (!settings->contains("WidthMM"))
        settings->setValue("WidthMM", 89.0);
    if (!settings->contains("HeightMM"))
        settings->setValue("HeightMM", 36.0);
    settings->endGroup();

    /* Display Data */
    settings->beginGroup("Display");
    if (!settings->contains("FullScreen"))
        settings->setValue("FullScreen", false);
    if (!settings->contains("ShowVirtualKeyboard"))
        settings->setValue("ShowVirtualKeyboard", false);
    settings->endGroup();

    // Chip Reader
    settings->beginGroup("ChipReader");
    if (!settings->contains("Port"))
        settings->setValue("Port", "COM8");
    if (!settings->contains("BaudRate"))
        settings->setValue("BaudRate", "9600");
    if (!settings->contains("DataBits"))
        settings->setValue("DataBits", "8");
    if (!settings->contains("Parity"))
        settings->setValue("Parity", "0");
    if (!settings->contains("StopBits"))
        settings->setValue("StopBits", "1");
    if (!settings->contains("FlowControl"))
        settings->setValue("FlowControl", "0");
    settings->endGroup();

    // Barcode Reader
    settings->beginGroup("BarcodeReader");
    if (!settings->contains("Port"))
        settings->setValue("Port", "COM10");
    if (!settings->contains("BaudRate"))
        settings->setValue("BaudRate", "9600");
    if (!settings->contains("DataBits"))
        settings->setValue("DataBits", "8");
    if (!settings->contains("Parity"))
        settings->setValue("Parity", "0");
    if (!settings->contains("StopBits"))
        settings->setValue("StopBits", "1");
    if (!settings->contains("FlowControl"))
        settings->setValue("FlowControl", "0");
    settings->endGroup();

    /* Sound Data */
    settings->beginGroup("Sound");
    if (!settings->contains("Enabled"))
        settings->setValue("Enabled", false);
    if (!settings->contains("Volume"))
        settings->setValue("Volume", 50);
    settings->endGroup();

    /* Language Data */
    settings->beginGroup("Language");
    if (!settings->contains("Selected"))
        settings->setValue("Selected", "de_DE");
    settings->endGroup();

    // Communication
    settings->beginGroup("Communication");
    if (!settings->contains("LocalPort"))
        settings->setValue("LocalPort", 61233);
    settings->endGroup();

    // Company
    settings->beginGroup("Company");
    if (!settings->contains("Location"))
        settings->setValue("Location", "1");
    settings->endGroup();

    // Style
    settings->beginGroup("Style");
    if (!settings->contains("Selected"))
        settings->setValue("Selected", "0");
    settings->endGroup();

    // Order
    settings->beginGroup("Order");
    if (!settings->contains("ConfirmationFilePath"))
        settings->setValue("ConfirmationFilePath", "C:/ProgramData/AMS/OrderConfirmationFiles");
    settings->endGroup();

    // SSH
    settings->beginGroup("SSH");
    if (!settings->contains("Hostname"))
        settings->setValue("Hostname", "");
    if (!settings->contains("Port"))
        settings->setValue("Port", 22);
    if (!settings->contains("Username"))
        settings->setValue("Username", "");
    if (!settings->contains("ProgrammPath"))
        settings->setValue("ProgrammPath", "C:/SSHProgramm.exe");
    if (!settings->contains("IsActive"))
        settings->setValue("IsActive", false);
    if (!settings->contains("LocalPort"))
        settings->setValue("LocalPort", 3307);
    settings->endGroup();

    // Label Template
    settings->beginGroup("ActiveLabelTemplateNameOverride");
    if (!settings->contains("ActiveLabelTemplateName"))
        settings->setValue("ActiveLabelTemplateName", "");
    settings->endGroup();

    settings->beginGroup("TTS");
    if (!settings->contains("Active"))
        settings->setValue("Active", false);
    settings->endGroup();

    settings->beginGroup("CrashRotate");
    if (!settings->contains("Active"))
        settings->setValue("Active", true);
    if (!settings->contains("MaxFileKeep"))
        settings->setValue("MaxFileKeep", 10);
    if (!settings->contains("MaxDays"))
        settings->setValue("MaxDays", 7);
    settings->endGroup();

    // Network Share
    settings->beginGroup("NetworkShare");
    if (!settings->contains("NetworkPath"))
        settings->setValue("NetworkPath", "\\\\ams-filesvc");
    if (!settings->contains("NetworkUser"))
        settings->setValue("NetworkUser", "user-ams");
    if (!settings->contains("NetworkPassword"))
        settings->setValue("NetworkPassword", "Unused");
    if (!settings->contains("NetworkShareActive"))
        settings->setValue("NetworkShareActive", true);
    settings->endGroup();

    // Local File
    settings->beginGroup("LocalFileData");
    if (!settings->contains("RootPath"))
        settings->setValue("RootPath", "C:/AMS/files");
    if (!settings->contains("IsActive"))
        settings->setValue("IsActive", false);
    settings->endGroup();
}

void SettingsManager::LoadSettings()
{
    loadMySQLSettings();
    loadLabelLayoutSettings();
    loadSoundSettings();
    loadLanguage();
    loadFullScreen();
    loadPersonalStyleEnabled();
    loadPersonalLanguageEnabled();
    loadVirtualKeyboardEnabled();
    loadLocalCommunicationPort();
    loadLogPath();
    loadLogPrefix();
    loadStyleSelection();
    loadCompanyLocation();
    loadChipReader();
    loadBarcodeReader();
    loadSSHSettings();
    loadOrderConfirmationFilePath();
    loadEnabledLogLevels();
    loadWindowGeometry();
    loadLabelTemplateOverrideName();
    loadTTSActive();
    loadCrashRotateData();
    loadNetworkShareData();
    loadLocalFileData();

    checkFileHandling();
}

void SettingsManager::loadMySQLSettings()
{
    const std::optional<std::string> mySQLPasswordOpt = PasswordStore::LoadPassword();

    settings->beginGroup("MySQL");
    settings->beginGroup("IMS");
    // const std::optional<std::string> mySQLPasswordOpt = PasswordStore::LoadPassword();
    _mySQLSettings = MySQLSettings{settings->value("Hostname").toString().toStdString(),
                                   settings->value("Username").toString().toStdString(),
                                   mySQLPasswordOpt.value_or(""),
                                   settings->value("Port").toString().toStdString(),
                                   settings->value("Database").toString().toStdString(),
                                   settings->value("IsActive").toBool()};
    settings->endGroup();
    settings->beginGroup("AMS");
    // const std::optional<std::string> mySQLPasswordOpt = PasswordStore::LoadPassword();
    _amsSettings = MySQLSettings{settings->value("Hostname").toString().toStdString(),
                                 settings->value("Username").toString().toStdString(),
                                 mySQLPasswordOpt.value_or(""),
                                 settings->value("Port").toString().toStdString(),
                                 settings->value("Database").toString().toStdString(),
                                 settings->value("IsActive").toBool()};
    settings->endGroup();
    settings->endGroup();
}

void SettingsManager::loadLabelLayoutSettings()
{
    settings->beginGroup("LabelLayout");
    _labelLayoutSettings =
        LabelLayoutSettings{settings->value("WidthMM", 89.0).toDouble(), settings->value("HeightMM", 36.0).toDouble(),
                            settings->value("FontSizePt", 10.0).toDouble()};
    settings->endGroup();
}

void SettingsManager::loadSoundSettings()
{
    settings->beginGroup("Sound");
    const SoundSettings result{settings->value("Volume", 50).toInt(), settings->value("Enabled", true).toBool()};
    settings->endGroup();
    _soundSettings = result;
}

void SettingsManager::loadLanguage()
{
    settings->beginGroup("Language");
    _language = settings->value("Selected", "en_GB").toString().toStdString();
    settings->endGroup();
}

void SettingsManager::loadFullScreen()
{
    settings->beginGroup("Display");
    _isFullScreen = settings->value("FullScreen", false).toBool();
    settings->endGroup();
}

void SettingsManager::loadPersonalStyleEnabled()
{
    settings->beginGroup("Display");
    _personalStyleEnabled = settings->value("PersonalStyle", false).toBool();
    settings->endGroup();
}

void SettingsManager::loadPersonalLanguageEnabled()
{
    settings->beginGroup("Display");
    _personalLanguageEnabled = settings->value("PersonalLanguage", false).toBool();
    settings->endGroup();
}

void SettingsManager::loadVirtualKeyboardEnabled()
{
    settings->beginGroup("Display");
    _virtualKeyboardEnabled = settings->value("ShowVirtualKeyboard", false).toBool();
    settings->endGroup();
}

void SettingsManager::loadLocalCommunicationPort()
{
    settings->beginGroup("Communication");
    _localCommunicationPort = settings->value("LocalPort", "61233").toString().toStdString();
    settings->endGroup();
}

void SettingsManager::loadLogPath()
{
    settings->beginGroup("Logging");
    _logPath = CheckPath(settings->value("LogPath").toString().toStdString(), "log");
    settings->endGroup();
}

void SettingsManager::loadLogPrefix()
{
    settings->beginGroup("Logging");
    _logPrefix = "IMS";  // settings->value("Prefix").toString().toStdString();
    settings->endGroup();
}

void SettingsManager::loadStyleSelection()
{
    settings->beginGroup("Style");
    _styleSelection = settings->value("Selected", "0").toUInt();
    settings->endGroup();
}

void SettingsManager::loadCompanyLocation()
{
    settings->beginGroup("Company");
    int value = settings->value("Location", "1").toInt();
    settings->endGroup();
    _companyLocation = static_cast<CompanyLocations>(value);
}

void SettingsManager::loadChipReader()
{
    settings->beginGroup("ChipReader");
    _chipReader = SerialSettings{settings->value("Port").toString().toStdString(),
                                 static_cast<QSerialPort::BaudRate>(settings->value("BaudRate").toInt()),
                                 static_cast<QSerialPort::DataBits>(settings->value("DataBits").toInt()),
                                 static_cast<QSerialPort::Parity>(settings->value("Parity").toInt()),
                                 static_cast<QSerialPort::StopBits>(settings->value("StopBits").toInt()),
                                 static_cast<QSerialPort::FlowControl>(settings->value("FlowControl").toInt())};
    settings->endGroup();
}

void SettingsManager::loadBarcodeReader()
{
    settings->beginGroup("BarcodeReader");
    _barcodeReader = SerialSettings{settings->value("Port").toString().toStdString(),
                                    static_cast<QSerialPort::BaudRate>(settings->value("BaudRate").toInt()),
                                    static_cast<QSerialPort::DataBits>(settings->value("DataBits").toInt()),
                                    static_cast<QSerialPort::Parity>(settings->value("Parity").toInt()),
                                    static_cast<QSerialPort::StopBits>(settings->value("StopBits").toInt()),
                                    static_cast<QSerialPort::FlowControl>(settings->value("FlowControl").toInt())};
    settings->endGroup();
}

void SettingsManager::loadSSHSettings()
{
    settings->beginGroup("SSH");
    _sshSettings = SSHSettings{settings->value("Hostname").toString().toStdString(),
                               settings->value("Port").toInt(),
                               settings->value("Username").toString().toStdString(),
                               settings->value("ProgrammPath").toString().toStdString(),
                               settings->value("IsActive").toBool(),
                               settings->value("LocalPort").toInt()};
    settings->endGroup();
}

void SettingsManager::loadOrderConfirmationFilePath()
{
    settings->beginGroup("Order");
    _orderConfirmationFilePath =
        CheckPath(settings->value("ConfirmationFilePath").toString().toStdString(), "confirmationFiles");
    settings->endGroup();
}

void SettingsManager::loadEnabledLogLevels()
{
    // Define all levels you support
    static constexpr LogLevel kAll[] = {LogLevel::LOG_LEVEL_TRACE, LogLevel::LOG_LEVEL_DEBUG,
                                        LogLevel::LOG_LEVEL_INFO,  LogLevel::LOG_LEVEL_WARNING,
                                        LogLevel::LOG_LEVEL_ERROR, LogLevel::LOG_LEVEL_FATAL};

    settings->beginGroup("Logging");
    const QVariant v = settings->value("EnabledLogLevel");
    settings->endGroup();

    QStringList list;
    if (v.metaType().id() == QMetaType::QStringList)
        list = v.toStringList();
    else
        list = v.toString().split(',', Qt::SkipEmptyParts);  // backward compat

    QSet<int> enabled;
    for (const QString& s : list)
        enabled.insert(s.toInt());

    std::unordered_map<LogLevel, bool> out;
    out.reserve(std::size(kAll));
    for (LogLevel lv : kAll)
        out.emplace(lv, enabled.contains(static_cast<int>(lv)));

    _enabledLogLevels = out;
}

void SettingsManager::loadWindowGeometry()
{
    settings->beginGroup("Window");

    for (int i = static_cast<int>(WindowNames::WINDOW_MAIN_WINDOW);
         i <= static_cast<int>(WindowNames::WINDOW_BOX_MANAGER); ++i)
    {
        auto window = static_cast<WindowNames>(i);
        const QString key = QString::fromStdString(GetWindowNameString(window));

        settings->beginGroup(key);
        int w = settings->value("Width", 800).toInt();
        int h = settings->value("Height", 600).toInt();
        int x = settings->value("X", 100).toInt();
        int y = settings->value("Y", 100).toInt();
        settings->endGroup();

        _windowGeometry[window] = {QSize(w, h), QPoint(x, y)};
    }

    settings->endGroup();
}

void SettingsManager::loadLabelTemplateOverrideName()
{
    settings->beginGroup("ActiveLabelTemplateNameOverride");
    _labelTemplateOverrideName = settings->value("ActiveLabelTemplateName", "").toString().toStdString();
    settings->endGroup();
}

void SettingsManager::loadTTSActive()
{
    settings->beginGroup("TTS");
    _isTTSActive = settings->value("Active", false).toBool();
    settings->endGroup();
}

void SettingsManager::loadCrashRotateData()
{
    settings->beginGroup("CrashRotate");
    _crashRotateData.isActive = settings->value("Active", false).toBool();
    _crashRotateData.maxCrashDumps = settings->value("MaxFileKeep", 10).toUInt();
    _crashRotateData.maxDays = settings->value("MaxDays", 7).toUInt();
    settings->endGroup();
}

void SettingsManager::loadNetworkShareData()
{
    settings->beginGroup("NetworkShare");
    _networkShareData.networkShareUser = settings->value("NetworkUser", "user_ams").toString().trimmed();
    _networkShareData.networkSharePath = settings->value("NetworkPath", "\\\\ams-filesvc").toString().trimmed();
    _networkShareData.networkShareActive = settings->value("NetworkShareActive", true).toBool();
    settings->endGroup();

    _networkShareData.networkSharePassword = "3C2öM6,fZrtN-}7";

    auto pw = NetworkSharePasswordStore::LoadPassword();
    if (pw.has_value())
        _networkShareData.networkSharePassword = QString::fromStdString(*pw);
}

void SettingsManager::loadLocalFileData()
{
    settings->beginGroup("LocalFileData");
    _localFileData.rootPath = settings->value("RootPath", "C:/AMS/files").toString().trimmed();
    _localFileData.isActive = settings->value("IsActive", false).toBool();
    settings->endGroup();
}

void SettingsManager::HandleGlobalSetting(const GlobalSettings setting)
{
    switch (setting)
    {
        case GlobalSettings::GLOBAL_SETTING_ORDER_FILE_PATH:
            LoadGlobalOrderFilePath();
            break;
        case GlobalSettings::GLOBAL_SETTING_ORDER_FILE_FOLDER:
            LoadGlobalOrderFolder();
            break;
        case GlobalSettings::GLOBAL_SETTING_AMS_LOCAL_FILE_ROOT:
            LoadGlobalLocalFileData();
            break;
        case GlobalSettings::GLOBAL_SETTING_MAX:
        default:
            break;
    }
}

void SettingsManager::LoadGlobalLocalFileData()
{
    // SELECT Data, DataType FROM global_settings WHERE SettingsKey = ?

    ConnectionGuardIMS connection(ConnectionType::Async);

    auto stmt = connection->GetPreparedStatement(IMSPreparedStatement::DB_GS_LOAD_LOCAL_FILE_ROOT_PATH);
    stmt->SetUInt(0, static_cast<std::uint16_t>(GlobalSettings::GLOBAL_SETTING_AMS_LOCAL_FILE_ROOT));

    auto result = connection->ExecutePreparedSelect(*stmt);

    if (!result.IsValid())
        return;

    while (result.Next())
    {
        Field* fields = result.Fetch();

        const QString data = fields[0].ToQString();
        std::uint8_t type = fields[1].GetUInt8();

        if (type == static_cast<std::uint8_t>(GlobalSettingsTypeLocalFile::DATA_TYPE_ROOT_PATH))
            _localFileData.rootPath = data;
        else if (type == static_cast<std::uint8_t>(GlobalSettingsTypeLocalFile::DATA_TYPE_IS_ACTIVE))
            _localFileData.isActive = true;
        else if (type == static_cast<std::uint8_t>(GlobalSettingsTypeLocalFile::DATA_TYPE_DEACTIVATED))
            _localFileData.isActive = false;
    }

    checkFileHandling();
}

void SettingsManager::checkFileHandling()
{
    if (_networkShareData.networkShareActive && _localFileData.isActive)
        _networkShareData.networkShareActive = false;
}

std::string SettingsManager::CheckPath(const std::string& inputPath, const std::string& lastFolderName) const
{
    try
    {
        const std::filesystem::path pathObj(inputPath);

        // Check whether the drive of the path exists.
        if (!std::filesystem::exists(pathObj.root_path()))
        {
            // The drive does not exist, return the root directory with lastFolderName.
            BootstrapLogger::Log("the driver of path not exist, give the base path back: " +
                                 (std::filesystem::current_path() / lastFolderName).string());
            return (std::filesystem::current_path() / lastFolderName).string();
        }

        // Check whether the specified path is a directory.
        if (!std::filesystem::is_directory(pathObj))
        {
            // The specified path is not a directory, create it.
            if (!std::filesystem::create_directories(pathObj))
            {
                // The directory could not be created, return the root directory with lastFolderName.
                BootstrapLogger::Log(
                    " The directory could not be created, return the root directory with lastFolderName: " +
                    (std::filesystem::current_path() / lastFolderName).string());
                return (std::filesystem::current_path() / lastFolderName).string();
            }
        }

        // The path exists and is a directory.
        return inputPath;
    }
    catch (const std::exception& e)
    {
        BootstrapLogger::Log("Exception occurred: " + std::string(e.what()));
        return "";
    }
}

SettingsManager::SettingsManager()
    : _mySQLSettings(),
      _labelLayoutSettings(),
      _soundSettings(),
      _isFullScreen(false),
      _personalStyleEnabled(false),
      _personalLanguageEnabled(false),
      _virtualKeyboardEnabled(false),
      _styleSelection(0),
      _companyLocation(),
      _chipReader(),
      _barcodeReader(),
      _sshSettings()
{
    const QString iniPath = "C:/ProgramData/AMS/settings.ini";
    QDir().mkpath(QFileInfo(iniPath).absolutePath());

    settings = std::make_unique<QSettings>(iniPath, QSettings::IniFormat);
    _crypto = std::make_unique<Crypto>();

    const bool fileMissing = !QFileInfo::exists(iniPath);
    const bool fileEmpty = settings->allKeys().isEmpty();

    if (fileMissing || fileEmpty)
    {
        InitDefaults();
        settings->sync();
    }
}

void SettingsManager::setIsFullScreen(const bool isActive /*= false*/) const
{
    settings->beginGroup("Display");
    settings->setValue("FullScreen", isActive);
    settings->endGroup();
}

void SettingsManager::SetPersonalStyleEnabled(const bool isActive /*= false*/)
{
    settings->beginGroup("Display");
    settings->setValue("PersonalStyle", isActive);
    settings->endGroup();

    loadPersonalStyleEnabled();
}

void SettingsManager::SetPersonalLanguageEnabled(const bool isActive /*= false*/)
{
    settings->beginGroup("Display");
    settings->setValue("PersonalLanguage", isActive);
    settings->endGroup();

    loadPersonalLanguageEnabled();
}

void SettingsManager::SetVirtualKeyboardEnabled(const bool isActive /*= false*/) const
{
    settings->beginGroup("Display");
    settings->setValue("ShowVirtualKeyboard", isActive);
    settings->endGroup();
}

void SettingsManager::SetTTSActive(bool isActive /*= false*/) const
{
    settings->beginGroup("TTS");
    settings->setValue("Active", isActive);
    settings->endGroup();
}

void SettingsManager::SetCrashRotateData(const CrashRotateData& crashRotateData) const
{
    settings->beginGroup("CrashRotate");
    settings->setValue("Active", crashRotateData.isActive);
    settings->setValue("MaxFileKeep", crashRotateData.maxCrashDumps);
    settings->setValue("MaxDays", crashRotateData.maxDays);
    settings->endGroup();
}

void SettingsManager::SetNetworkShareData(const NetworkShareData& networkData) const
{
    settings->beginGroup("NetworkShare");
    settings->setValue("NetworkPath", networkData.networkSharePath);
    settings->setValue("NetworkUser", networkData.networkShareUser);
    settings->setValue("NetworkShareActive", networkData.networkShareActive);
    settings->endGroup();

    if (!NetworkSharePasswordStore::SavePassword(networkData.networkSharePassword.toStdString()))
        LOG_ERROR("Failed to store network share password in Windows Credential Store");
}

void SettingsManager::SetLocalFileData(const LocalFileData& localFileData) const
{
    settings->beginGroup("LocalFileData");
    settings->setValue("RootPath", localFileData.rootPath);
    settings->setValue("IsActive", localFileData.isActive);
    settings->endGroup();
}

void SettingsManager::LoadGlobalSettings()
{
    for (auto i = static_cast<std::uint16_t>(GlobalSettings::GLOBAL_SETTING_ORDER_FILE_PATH);
         i < static_cast<std::uint16_t>(GlobalSettings::GLOBAL_SETTING_MAX); i++)
        HandleGlobalSetting(static_cast<GlobalSettings>(i));
}

void SettingsManager::LoadGlobalOrderFilePath()
{
    // Not needed in IMS
}

void SettingsManager::LoadGlobalOrderFolder()
{
    // Not needed in IMS
}

void SettingsManager::SynSettings()
{
    settings->sync();

    if (settings->status() != QSettings::NoError)
        BootstrapLogger::Log("QSettings error while writing settings.ini");
}

void SettingsManager::SetLabelTemplateOverrideName(const std::string& labelTemplateOverrideName)
{
    settings->beginGroup("ActiveLabelTemplateNameOverride");
    settings->setValue("ActiveLabelTemplateName", QString::fromStdString(labelTemplateOverrideName));
    settings->endGroup();

    loadLabelTemplateOverrideName();
}

WindowGeometry SettingsManager::getWindowGeometry(const WindowNames name) const
{
    if (const auto windowData = _windowGeometry.find(name); windowData != _windowGeometry.end())
        return windowData->second;

    return WindowGeometry{QSize(800, 600), QPoint(0, 0)};
}

void SettingsManager::setWindowGeometry(const WindowNames name, const WindowGeometry& geo) const
{
    settings->beginGroup("Window");
    settings->beginGroup(QString::fromStdString(GetWindowNameString(name)));

    settings->setValue("Width", geo.size.width());
    settings->setValue("Height", geo.size.height());
    settings->setValue("X", geo.position.x());
    settings->setValue("Y", geo.position.y());

    settings->endGroup();  // Window group
    settings->endGroup();  // "Window"
}

void SettingsManager::SetMySQLData(const MySQLSettings& mySQLSetting) const
{
    settings->beginGroup("MySQL");
    settings->beginGroup("IMS");
    settings->setValue("Hostname", QString::fromStdString(mySQLSetting.hostname));
    settings->setValue("Username", QString::fromStdString(mySQLSetting.username));
    settings->setValue("Port", QString::fromStdString(mySQLSetting.port));
    settings->setValue("Database", QString::fromStdString(mySQLSetting.database));
    settings->setValue("IsActive", mySQLSetting.isActive);
    settings->endGroup();
    settings->endGroup();
}

void SettingsManager::SetAMSMySQLData(const MySQLSettings& amsMySQLSetting) const
{
    settings->beginGroup("MySQL");
    settings->beginGroup("AMS");
    settings->setValue("Hostname", QString::fromStdString(amsMySQLSetting.hostname));
    settings->setValue("Username", QString::fromStdString(amsMySQLSetting.username));
    settings->setValue("Port", QString::fromStdString(amsMySQLSetting.port));
    settings->setValue("Database", QString::fromStdString(amsMySQLSetting.database));
    settings->setValue("IsActive", amsMySQLSetting.isActive);
    settings->endGroup();
    settings->endGroup();
}

void SettingsManager::SetLabelLayout(const LabelLayoutSettings& labelLayout)
{
    settings->beginGroup("LabelLayout");
    settings->setValue("WidthMM", labelLayout.widthMM);
    settings->setValue("HeightMM", labelLayout.heightMM);
    settings->setValue("FontSizePt", labelLayout.fontSizePt);
    settings->endGroup();

    loadLabelLayoutSettings();
}

void SettingsManager::SetSoundSettings(const SoundSettings& soundSettings)
{
    settings->beginGroup("Sound");
    settings->setValue("Enabled", soundSettings.enabled);
    settings->setValue("Volume", soundSettings.volume);
    settings->endGroup();

    loadSoundSettings();
}

void SettingsManager::SetCompanyLocation(const CompanyLocations& companyLocation)
{
    settings->beginGroup("Company");
    settings->setValue("Location", static_cast<int>(companyLocation));
    settings->endGroup();

    loadCompanyLocation();
}

void SettingsManager::SetChipReader(const SerialSettings& chipReader) const
{
    settings->beginGroup("ChipReader");
    settings->setValue("Port", QString::fromStdString(chipReader.portName));
    settings->setValue("BaudRate", static_cast<int>(chipReader.baudRate));
    settings->setValue("DataBits", static_cast<int>(chipReader.dataBits));
    settings->setValue("Parity", static_cast<int>(chipReader.parity));
    settings->setValue("StopBits", static_cast<int>(chipReader.stopBits));
    settings->setValue("FlowControl", static_cast<int>(chipReader.flowControl));
    settings->endGroup();
}

void SettingsManager::SetBarcodeReader(const SerialSettings& barcodeReader) const
{
    settings->beginGroup("BarcodeReader");
    settings->setValue("Port", QString::fromStdString(barcodeReader.portName));
    settings->setValue("BaudRate", static_cast<int>(barcodeReader.baudRate));
    settings->setValue("DataBits", static_cast<int>(barcodeReader.dataBits));
    settings->setValue("Parity", static_cast<int>(barcodeReader.parity));
    settings->setValue("StopBits", static_cast<int>(barcodeReader.stopBits));
    settings->setValue("FlowControl", static_cast<int>(barcodeReader.flowControl));
    settings->endGroup();
}

void SettingsManager::SetLanguage(const std::string& language)
{
    settings->beginGroup("Language");
    settings->setValue("Selected", QString::fromStdString(language));
    settings->endGroup();

    loadLanguage();
}

void SettingsManager::SetLogPath(const std::string& logPath) const
{
    settings->beginGroup("Logging");
    settings->setValue("LogPath", QString::fromStdString(logPath));
    settings->endGroup();
}

void SettingsManager::SetLogEnabledDisabled(const std::unordered_map<LogLevel, bool>& enabledLogLevels)
{
    settings->beginGroup("Logging");

    // Collect enabled levels; store as QStringList
    QStringList levels;
    levels.reserve(static_cast<int>(enabledLogLevels.size()));
    for (const auto& [level, enabled] : enabledLogLevels)
    {
        if (enabled)
            levels << QString::number(static_cast<int>(level));
    }
    std::ranges::sort(levels);  // stable file diffs

    settings->setValue("EnabledLogLevel", levels);
    settings->endGroup();

    loadEnabledLogLevels();
}

void SettingsManager::SetLogPrefix(const std::string& logPrefix) const
{
    settings->beginGroup("Logging");
    settings->setValue("Prefix", QString::fromStdString(logPrefix));
    settings->endGroup();
}

void SettingsManager::SetOrderConfirmationFilePath(const std::string& orderConfirmationFilePath) const
{
    settings->beginGroup("Order");
    settings->setValue("ConfirmationFilePath", QString::fromStdString(orderConfirmationFilePath));
    settings->endGroup();
}

void SettingsManager::SetStyleSelection(const std::uint8_t styleSelection) const
{
    settings->beginGroup("Style");
    settings->setValue("Selected", styleSelection);
    settings->endGroup();
}

std::string SettingsManager::GetStyleString(const Styles styles)
{
    switch (static_cast<Styles>(styles))
    {
        case Styles::STYLE_LIGHT:
            return "light";
        case Styles::STYLE_DARK:
            return "dark";
        default:
            return "light";
    }
}

std::string SettingsManager::GetStyleString() const
{
    switch (static_cast<Styles>(getStyleSelection()))
    {
        case Styles::STYLE_LIGHT:
            return "light";
        case Styles::STYLE_DARK:
            return "dark";
        default:
            return "light";
    }
}

bool SettingsManager::isLogLevelEnabled(const LogLevel level) const
{
    const auto it = _enabledLogLevels.find(level);
    return it != _enabledLogLevels.end() && it->second;
}
