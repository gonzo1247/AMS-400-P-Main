#pragma once
#include <QMessageBox>
#include <oclero/qlementine.hpp>

#include "Util.h"
class MessageBoxHelper
{
public:
    MessageBoxHelper() = delete;
    ~MessageBoxHelper() = delete;

	static void ShowInfoMessage(const QString& message, bool exec = false, const QString &title = "Information");
    static void ShowInfoMessage(const std::string& message, bool exec = false, const std::string& title = "Information");
    static void ShowInfoMessage(ErrorCodes codes, bool exec = false, const QString& title = "Information");
    
	static void ShowErrorMessage(const QString& message, bool exec = false, const QString& title = "Error");
    static void ShowErrorMessage(const std::string& message, bool exec = false, const std::string& title = "Error");
    static void ShowErrorMessage(ErrorCodes codes, bool exec = false, const QString& title = "Error");
    
	static void ShowWarningMessage(const QString& message, bool exec = false, const QString& title = "Warning");
    static void ShowWarningMessage(const std::string& message, bool exec = false, const std::string& title = "Warning");
    static void ShowWarningMessage(ErrorCodes codes, bool exec = false, const QString& title = "Warning");

    static void ShowSuccessfullyMessage(const QString& message, bool exec = false, const QString& title = "Success");
    static void ShowSuccessfullyMessage(const std::string& message, bool exec = false, const std::string& title = "Success");
    static void ShowSuccessfullyMessage(SuccessfullCodes codes, bool exec = false, const QString& title = "Success");

private:
    static QMessageBox* CreateMessageBox(QMessageBox::Icon icon, const QString &message, const QString &title, QWidget* parent = nullptr);
    static QMessageBox* CreateMessageBox(const QIcon& icon, const QString &message, const QString &title, QWidget* parent = nullptr);
    static void ShowBox(QMessageBox *msg, bool exec);
    static QString LoadErrorCodeMessage(const ErrorCodes codes);
    static QString LoadSuccessCodeMessage(const SuccessfullCodes codes);
    static QString LoadMessageFromDatabase(int code);
};

