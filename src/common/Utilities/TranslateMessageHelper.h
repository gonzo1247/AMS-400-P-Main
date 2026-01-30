#pragma once
#include "Util.h"
class TranslateMessageHelper
{
public:
	TranslateMessageHelper() = delete;
    ~TranslateMessageHelper() = delete;

	static QString LoadErrorCodeMessage(const ErrorCodes& code);
	static QString LoadSuccessCodeMessage(const SuccessfullCodes& code);

private:
    static QString LoadMessageFromDatabase(int code);
};

