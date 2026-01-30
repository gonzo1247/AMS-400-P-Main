#include "pch.h"

#include "MessageBoxHelper.h"
#include "ConnectionGuard.h"
#include "DatabaseTypes.h"

#include <oclero/qlementine/icons/QlementineIcons.hpp>
#include <oclero/qlementine/icons/Icons16.hpp>


void MessageBoxHelper::ShowInfoMessage(const QString &message, const bool exec /*= false*/, const QString &title /*= "Information"*/)
{
    ShowBox(CreateMessageBox(QMessageBox::Information, message, title), exec);
}

void MessageBoxHelper::ShowInfoMessage(const std::string &message, const bool exec /*= false*/, const std::string &title /*= "Information"*/)
{
    ShowInfoMessage(QString::fromStdString(message), exec, QString::fromStdString(title));
}

void MessageBoxHelper::ShowInfoMessage(const ErrorCodes codes, const bool exec /*= false*/, const QString &title /*= "Information"*/)
{
     ShowInfoMessage(LoadErrorCodeMessage(codes), exec, title);
}

void MessageBoxHelper::ShowErrorMessage(const QString &message, const bool exec /*= false*/, const QString &title /*= "Error"*/)
{
    ShowBox(CreateMessageBox(QMessageBox::Critical, message, title), exec);
}

void MessageBoxHelper::ShowErrorMessage(const std::string &message, const bool exec /*= false*/, const std::string &title /*= "Error"*/)
{
    ShowErrorMessage(QString::fromStdString(message), exec, QString::fromStdString(title));
}

void MessageBoxHelper::ShowErrorMessage(ErrorCodes codes, bool exec /*= false*/, const QString &title /*= "Error"*/)
{
    ShowErrorMessage(LoadErrorCodeMessage(codes), exec, title);
}

auto MessageBoxHelper::ShowWarningMessage(const QString &message, const bool exec /*= false*/, const QString &title /*= "Warning"*/) -> void
{
    ShowBox(CreateMessageBox(QMessageBox::Warning, message, title), exec);
}

void MessageBoxHelper::ShowWarningMessage(const std::string &message, const bool exec /*= false*/, const std::string &title /*= "Warning"*/)
{     
    ShowWarningMessage(QString::fromStdString(message), exec, QString::fromStdString(title));
}

void MessageBoxHelper::ShowWarningMessage(const ErrorCodes codes, const bool exec /*= false*/, const QString &title /*= "Warning"*/)
{
    ShowWarningMessage(LoadErrorCodeMessage(codes), exec, title);
}

void MessageBoxHelper::ShowSuccessfullyMessage(const QString &message, bool exec /*= false*/, const QString &title /*= "Success"*/)
{
    auto icon = Util::QlementineIconsMaker(oclero::qlementine::icons::Icons16::Misc_Success, {20, 20}, QColor("#38e065"));
    ShowBox(CreateMessageBox(icon, message, title), exec);
}

void MessageBoxHelper::ShowSuccessfullyMessage(const std::string &message, bool exec /*= false*/, const std::string &title /*= "Success"*/)
{
    ShowSuccessfullyMessage(QString::fromStdString(message), exec, QString::fromStdString(title));
}

void MessageBoxHelper::ShowSuccessfullyMessage(SuccessfullCodes codes, bool exec /*= false*/, const QString &title /*= "Success"*/)
{
    ShowSuccessfullyMessage(LoadSuccessCodeMessage(codes), exec, title);
}

QMessageBox *MessageBoxHelper::CreateMessageBox(const QMessageBox::Icon icon, const QString &message, const QString &title, QWidget* parent /*= nullptr*/)
{
    auto msg = new QMessageBox(parent);
    msg->setIcon(icon);
    msg->setWindowTitle(title);
    msg->setText(message);
    msg->setStandardButtons(QMessageBox::Ok);
    return msg;
}

QMessageBox *MessageBoxHelper::CreateMessageBox(const QIcon &icon, const QString &message, const QString &title, QWidget *parent /*= nullptr*/)
{
    auto msg = new QMessageBox(parent);
    msg->setIconPixmap(icon.pixmap(64,64));
    msg->setWindowTitle(title);
    msg->setText(message);
    msg->setStandardButtons(QMessageBox::Ok);
    return msg;
}

void MessageBoxHelper::ShowBox(QMessageBox *msg, const bool exec)
{
    if (!msg)
        return;

    if (!exec)
    {
        msg->setAttribute(Qt::WA_DeleteOnClose);
        msg->show();
    }
    else
    {
        msg->exec();
        delete msg;
    }
}

QString MessageBoxHelper::LoadErrorCodeMessage(const ErrorCodes codes)
{
    return LoadMessageFromDatabase(static_cast<int>(codes));
}

QString MessageBoxHelper::LoadSuccessCodeMessage(const SuccessfullCodes codes)
{
    return LoadMessageFromDatabase(static_cast<int>(codes));
}

QString MessageBoxHelper::LoadMessageFromDatabase(int code)
{
    ConnectionGuardIMS guard(database::ConnectionType::Sync);

    auto message = guard->GetPreparedStatement(IMSPreparedStatement::DB_MT_SELECT_TRANSLATION_BY_LANGUAGE_AND_KEY);
    message->SetString(0, GetSettings().getLanguage());
    message->SetInt(1, code);

    auto result = guard->ExecutePreparedSelect(*message);

    if (!result.IsValid())
        return {};

    if (result.Next())
    {
        Field *fields = result.Fetch();
        return QString::fromUtf8(fields[0].GetString());
    }

    return {};
}
