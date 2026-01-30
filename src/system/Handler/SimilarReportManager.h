// SimilarReportManager.h
#pragma once

#include <QString>
#include <cstdint>
#include <string>
#include <vector>

#include "ConnectionGuard.h"

struct SimilarReportResult
{
    std::uint64_t ticketId = 0;
    QString title;
    QString updatedAt;
    double score = 0.0;
};

class SimilarReportManager
{
   public:
    explicit SimilarReportManager();

    std::vector<SimilarReportResult> FindSimilarTickets(std::uint64_t ticketId, std::uint32_t limit = 10);

   private:
    struct SimilarSearchInput
    {
        std::string title;
        std::string description;
        std::string reportPlain;
        std::optional<std::uint32_t> entityId;
    };

    struct SimilarSearchTokens
    {
        std::string fulltextBooleanQuery;
        std::vector<std::string> strongTokens;
    };

    struct SimilarQuery
    {
        QString sql;
        int strongTokenCount = 0;
    };

   private:
    SimilarSearchInput LoadSearchInput(std::uint64_t ticketId, const ConnectionGuardAMS& connection);
    SimilarSearchTokens BuildSimilarSearchTokens(const SimilarSearchInput& in) const;
    SimilarQuery BuildSimilarTicketsSql(const std::string& ftQuery, int maxStrongTokens, bool hasEntity) const;
    QString EscapeSqlLiteral(QString s) const;

};
