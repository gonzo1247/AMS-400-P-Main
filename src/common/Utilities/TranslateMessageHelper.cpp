#include "TranslateMessageHelper.h"

#include "ConnectionGuard.h"
#include "pch.h"

QString TranslateMessageHelper::LoadErrorCodeMessage(const ErrorCodes& code)
{
    return LoadMessageFromDatabase(static_cast<int>(code));
}

QString TranslateMessageHelper::LoadSuccessCodeMessage(const SuccessfullCodes& code)
{
    return LoadMessageFromDatabase(static_cast<int>(code));
}

QString TranslateMessageHelper::LoadMessageFromDatabase(int code)
{
    ConnectionGuardIMS guard(ConnectionType::Sync);

    auto message = guard->GetPreparedStatement(IMSPreparedStatement::DB_MT_SELECT_TRANSLATION_BY_LANGUAGE_AND_KEY);
    message->SetString(0, GetSettings().getLanguage());
    message->SetInt(1, code);

    auto result = guard->ExecutePreparedSelect(*message);

    if (!result.IsValid())
        return {};

    if (result.Next())
    {
        Field* fields = result.Fetch();
        return QString::fromUtf8(fields[0].GetString());
    }

    return {};
}
