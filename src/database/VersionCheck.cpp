#include "pch.h"
#include "VersionCheck.h"
#include <QMessageBox>
#include <QString>
#include <chrono>

#include "ConnectionGuard.h"
#include "Databases.h"
#include "VersionExit.h"

using namespace std::chrono_literals;

bool VersionCheck::CheckCompatibilityWithDB(QWidget* parent)
{
	const auto rowOpt = LoadActiveVersionFor("AMS");

	if (!rowOpt)
	{
		QMessageBox::critical(parent, QObject::tr("Version check failed"), QObject::tr("Failed to load active version information from database."));
		return false;
	}

	const auto& row = *rowOpt;

	// Allow dev bypass only when explicitly enabled in DB
	const bool compatible = (kClientCode >= row.min_client_code) && (kClientCode <= row.max_client_code);

	if (compatible || row.bypass_active)
	{
		if (row.bypass_active)
		{
			// Optional: log that bypass is active
			LOG_DEBUG("Version bypass is active for component; clientCode={}, allowed=[{}, {}]",
				kClientCode, row.min_client_code, row.max_client_code);
		}
		return true;
	}

	const QString msg = QStringLiteral(
		"Database version %1 is active.\n\n"
		"This installation (%2.%3.%4-%5) is not compatible.\n"
		"Please update the software or contact support (Weitz (165) / Winter (166).")
		.arg(QString::fromStdString(row.version_str))
		.arg(APP_VER_MAJOR)
		.arg(APP_VER_PATCH)
		.arg(APP_VER_HOTFIX)
		.arg(APP_VER_BUILD);

	VersionExit::AbortApp(parent, msg);
	LOG_DEBUG("Version incompatible: clientCode={}, allowed=[{}, {}], dbActive={}",
		kClientCode, row.min_client_code, row.max_client_code, row.version_code);

	return false;
}

std::optional<ActiveVersionRow> VersionCheck::LoadActiveVersionFor(const std::string& component) // 
{
    ConnectionGuardIMS guard(ConnectionType::Sync);

	std::optional<ActiveVersionRow> versionsInfo;

	//			1				2				3				4				5		
	// SELECT version_code, min_client_code, max_client_code, bypass_active, version_str "
	//	FROM app_version "
	//	WHERE component = ? AND is_active = 1 "
	//	LIMIT 1


	auto stmt = guard->GetPreparedStatement(IMSPreparedStatement::DB_VS_SELECT_ACTIVE_VERSION_INFO);
	stmt->SetString(1, component);
	auto result = guard->ExecutePreparedSelect(*stmt);

	if (result.IsValid())
		return std::nullopt;

	if (result.NextRow())
	{
        Field* field = result.Fetch();

		ActiveVersionRow row;
		row.version_code	= field[0].Get<std::uint64_t>(); //  result->getUInt64(1);
		row.min_client_code = field[1].Get<std::uint64_t>(); // result->getUInt64(2);
		row.max_client_code = field[2].Get<std::uint64_t>();  //result->getUInt64(3);
		row.bypass_active	= field[3].Get<std::uint64_t>();  //result->getBoolean(4);
		row.version_str		= field[4].ToString();  //result->getString(5);
		
		versionsInfo = std::move(row);
	}

	return versionsInfo;
}
