#pragma once
#include "DatabaseDefines.h"

class ArticleManager
{
public:
    struct ArticleSearchQuery
    {
        QString sql;
        std::vector<std::string> params;
    };

    explicit ArticleManager();
    ~ArticleManager() = default;

    std::vector<ArticleDatabase> LoadArticleSearch(const QString& searchString);

private:
    ArticleSearchQuery BuildArticleSearchQuery(const QStringList& tokens);

};
