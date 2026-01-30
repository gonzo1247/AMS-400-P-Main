#include "pch.h"
#include "ArticleManager.h"

#include "ConnectionGuard.h"
#include "DatabaseTypes.h"
#include "Util.h"

ArticleManager::ArticleManager() {}

std::vector<ArticleDatabase> ArticleManager::LoadArticleSearch(const QString& searchString)
 {
    QStringList tokens = Util::TokenizeSearch(searchString);

    if (tokens.isEmpty())
        return {};
    
    ArticleSearchQuery query = BuildArticleSearchQuery(tokens);

    ConnectionGuardIMS connection(database::ConnectionType::Sync);

    auto result = connection->ExecuteAdhocPreparedSelect(query.sql.toStdString(), query.params);

    if (!result.IsValid())
        return {};

    std::vector<ArticleDatabase> rows;

    rows.clear();
    rows.reserve(128);

    while (result.Next())
    {
        Field* f = result.Fetch();

        ArticleDatabase r;
        r.ID = f[0].GetUInt32();
        r.articleName = f[1].GetString();
        r.articleNumber = f[2].GetString();
        r.manufacturer = f[3].GetString();
        r.suppliedBy = f[4].GetString();

        rows.push_back(std::move(r));
    }

    return rows;
 }

ArticleManager::ArticleSearchQuery ArticleManager::BuildArticleSearchQuery(const QStringList& tokens)
{
     ArticleSearchQuery q;

     q.sql =
        "SELECT "
         "ID, "         
         "ArticleName, "
         "ArticleNumber, "
         "Manufacturer, "
         "SuppliedBy "
         "FROM article_database "
         "WHERE IsDeleted = 0 ";

     // AND over tokens, OR over columns
     for (const auto& token : tokens)
     {
         q.sql +=
             " AND ("
             "ArticleName LIKE ? OR "
             "ArticleNumber LIKE ? OR "
             "ReplacementPartNumber LIKE ? OR "
             "Manufacturer LIKE ? OR "
             "SuppliedBy LIKE ?"
             ")";

         const std::string pattern = "%" + token.toStdString() + "%";
         for (std::size_t i = 0; i < 5; ++i)
             q.params.push_back(pattern);
     }

     q.sql += " ORDER BY ArticleName LIMIT 200";

     return q;
 }
