#include "pch.h"

#include <QDateTime>
#include <QString>
#include <algorithm>
#include <cctype>
#include <regex>
#include <stdexcept>
#include <string_view>
#include <unordered_set>

#include "SimilarReportManager.h"

#include "ConnectionGuard.h"
#include "DatabaseTypes.h"

namespace
{
    std::string ToLowerAscii(std::string s)
    {
        std::ranges::transform(s, s.begin(), [](unsigned char c) { return char(std::tolower(c)); });
        return s;
    }

    std::string KeepAlnum(std::string s)
    {
        std::erase_if(s, [](unsigned char c) { return !std::isalnum(c); });
        return s;
    }

    bool HasAlpha(std::string_view s)
    {
        return std::ranges::any_of(s, [](unsigned char c) { return std::isalpha(c); });
    }

    bool HasDigit(std::string_view s)
    {
        return std::ranges::any_of(s, [](unsigned char c) { return std::isdigit(c); });
    }

    bool IsStopword(std::string_view w)
    {
        static const std::unordered_set<std::string> stop{
            "der",    "die",    "das",  "und",   "oder", "mit",   "ohne",     "bei",     "von",    "zu",
            "im",     "in",     "am",   "an",    "auf",  "bitte", "dringend", "sofort",  "fehler", "problem",
            "defekt", "kaputt", "geht", "nicht", "kein", "keine", "meldung",  "service", "anlage", "maschine"};

        return stop.contains(std::string(w));
    }

    std::vector<std::string> ExtractWords(std::string_view input)
    {
        // Keeps words and allows separators (F-12, A_12, X/3).
        static const std::regex rx(R"([a-zA-Z0-9]+(?:[-_/][a-zA-Z0-9]+)*)");

        std::vector<std::string> out;
        const std::string s(input);

        for (auto it = std::sregex_iterator(s.begin(), s.end(), rx); it != std::sregex_iterator(); ++it)
            out.push_back((*it)[0].str());

        return out;
    }
}  // namespace

SimilarReportManager::SimilarReportManager() {}

std::vector<SimilarReportResult> SimilarReportManager::FindSimilarTickets(std::uint64_t ticketId, std::uint32_t limit)
{
    LOG_DEBUG("Similar: FindSimilarTickets(ticketId={}, limit={})", ticketId, limit);

    ConnectionGuardAMS connection(database::ConnectionType::Sync);

    if (!connection)
        return {};

    const SimilarSearchInput input = LoadSearchInput(ticketId, connection);

    const bool hasEntity = input.entityId.has_value() && *input.entityId != 0;

    const SimilarSearchTokens tokens = BuildSimilarSearchTokens(input);

    const int maxStrong = std::min<int>(int(tokens.strongTokens.size()), 8);
    const SimilarQuery q = BuildSimilarTicketsSql(tokens.fulltextBooleanQuery, maxStrong, hasEntity);

    LOG_DEBUG("Raw sql part: {}", q.sql);

    // ------------------------------------------------------------------
    // IMPORTANT: adapt this part to your AMS DB layer if needed.
    // You need a way to create a prepared statement from a raw SQL string.
    //
    // Examples seen in some codebases:
    // - auto stmt = _connection->PrepareStatement(q.sql.toStdString());
    // - auto stmt = _connection->CreatePreparedStatement(q.sql.toStdString());
    // - auto stmt = _connection->GetPreparedStatementFromSql(q.sql.toStdString());
    // ------------------------------------------------------------------

    auto stmt = connection->GetStatementRaw(q.sql.toStdString());

    int idx = 0;

    if (hasEntity)
        stmt->SetUInt(idx++, *input.entityId);  // score boost

    stmt->SetUInt64(idx++, ticketId);  // exclude self

    if (hasEntity)
        stmt->SetUInt(idx++, *input.entityId);  // WHERE OR entity

    // strong tokens (score)
    for (int i = 0; i < q.strongTokenCount; ++i)
    {
        const std::string& t = tokens.strongTokens[i];
        stmt->SetString(idx++, t);
        stmt->SetString(idx++, t);
        stmt->SetString(idx++, t);
    }

    // strong tokens (where)
    for (int i = 0; i < q.strongTokenCount; ++i)
    {
        const std::string& t = tokens.strongTokens[i];
        stmt->SetString(idx++, t);
        stmt->SetString(idx++, t);
        stmt->SetString(idx++, t);
    }

    stmt->SetUInt(idx++, limit);

    auto result = connection->ExecutePreparedSelect(*stmt);

    std::vector<SimilarReportResult> out;

    if (!result.IsValid())
        return out;

    while (result.Next())
    {
        Field* fields = result.Fetch();

        const auto foundId = fields[0].GetUInt64();
        if (foundId == ticketId)
            continue;

        SimilarReportResult r;
        r.ticketId = foundId;
        r.title = QString::fromStdString(fields[1].GetString());

        // updated_at can be DATETIME, depends on your Field impl.
        // If you already have a datetime formatter, use it here.
        if (!fields[2].IsNull())
        {
            const auto tp = fields[2].GetDateTime();
            const auto tt = std::chrono::system_clock::to_time_t(tp);
            r.updatedAt = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(tt)).toString(Qt::ISODate);
        }

        r.score = fields[3].GetDouble();

        out.push_back(std::move(r));
    }

    return out;
}

SimilarReportManager::SimilarSearchInput SimilarReportManager::LoadSearchInput(std::uint64_t ticketId, const ConnectionGuardAMS& connection)
{
    SimilarSearchInput out;

    {
        // tickets: title + description
        auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_TICKET_SELECT_TITLE_DESC_BY_ID);
        stmt->SetUInt64(0, ticketId);

        auto r = connection->ExecutePreparedSelect(*stmt);
        if (r.IsValid() && r.Next())
        {
            Field* f = r.Fetch();
            out.title = f[0].GetString();
            out.description = f[1].GetString();
            out.entityId = f[2].IsNull() ? std::nullopt : std::optional<std::uint32_t>(f[2].GetUInt32());
        }
    }

    {
        // ticket_report: report_plain (optional)
        auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_TICKET_REPORT_SELECT_PLAIN_BY_TICKET_ID);
        stmt->SetUInt64(0, ticketId);

        auto r = connection->ExecutePreparedSelect(*stmt);
        if (r.IsValid() && r.Next())
        {
            Field* f = r.Fetch();
            if (!f[0].IsNull())
                out.reportPlain = f[0].GetString();
        }
    }

    return out;
}

SimilarReportManager::SimilarSearchTokens SimilarReportManager::BuildSimilarSearchTokens(
    const SimilarSearchInput& in) const
{
    const std::string merged = ToLowerAscii(in.title + " " + in.description + " " + in.reportPlain);

    std::unordered_set<std::string> uniqWords;
    uniqWords.reserve(128);

    std::vector<std::string> strong;
    strong.reserve(16);

    for (auto w : ExtractWords(merged))
    {
        w = ToLowerAscii(std::move(w));
        w = KeepAlnum(std::move(w));

        if (w.size() < 3)
            continue;

        if (IsStopword(w))
            continue;

        if (!uniqWords.insert(w).second)
            continue;

        const bool isStrong = HasAlpha(w) && HasDigit(w);
        if (isStrong)
            strong.push_back(w);
    }

    std::sort(strong.begin(), strong.end(), [](const auto& a, const auto& b) { return a.size() > b.size(); });

    if (strong.size() > 8)
        strong.resize(8);

    std::vector<std::string> words;
    words.reserve(uniqWords.size());
    for (const auto& w : uniqWords)
        words.push_back(w);

    std::sort(words.begin(), words.end(), [](const auto& a, const auto& b) { return a.size() > b.size(); });

    if (words.size() > 12)
        words.resize(12);

    std::string ft;
    ft.reserve(256);

    for (const auto& w : words)
    {
        ft += '+';
        ft += w;
        ft += '*';
        ft += ' ';
    }

    SimilarSearchTokens out;
    out.fulltextBooleanQuery = std::move(ft);
    out.strongTokens = std::move(strong);
    return out;
}

SimilarReportManager::SimilarQuery SimilarReportManager::BuildSimilarTicketsSql(const std::string& ftQuery,
                                                                                int maxStrongTokens,
                                                                                bool hasEntity) const
{
    SimilarQuery q;

    QString strongScore;
    QString strongWhere;

    for (int i = 0; i < maxStrongTokens; ++i)
    {
        if (i > 0)
        {
            strongScore += " + ";
            strongWhere += " OR ";
        }

        strongScore += "(CASE WHEN LOWER(t.title) LIKE CONCAT('%', ?, '%') THEN 6 ELSE 0 END)";
        strongScore += " + ";
        strongScore += "(CASE WHEN LOWER(t.description) LIKE CONCAT('%', ?, '%') THEN 3 ELSE 0 END)";
        strongScore += " + ";
        strongScore += "(CASE WHEN LOWER(COALESCE(r.report_plain, '')) LIKE CONCAT('%', ?, '%') THEN 2 ELSE 0 END)";

        strongWhere += "LOWER(t.title) LIKE CONCAT('%', ?, '%')";
        strongWhere += " OR ";
        strongWhere += "LOWER(t.description) LIKE CONCAT('%', ?, '%')";
        strongWhere += " OR ";
        strongWhere += "LOWER(COALESCE(r.report_plain, '')) LIKE CONCAT('%', ?, '%')";
    }

    if (maxStrongTokens <= 0)
    {
        strongScore = "0";
        strongWhere = "0";
    }

    QString ft = QString::fromStdString(ftQuery);
    ft.replace('\'', "''");

    const QString entityScore = hasEntity ? "(CASE WHEN t.entity_id = ? THEN 500 ELSE 0 END) + " : "";
    const QString entityWhere = hasEntity ? "t.entity_id = ? OR " : "";

    q.sql = QString(R"SQL(
SELECT
    t.ID,
    t.title,
    t.updated_at,
    (
        %1
        (MATCH(t.title, t.description) AGAINST ('%2' IN BOOLEAN MODE) * 5.0) +
        (COALESCE(MATCH(r.report_plain) AGAINST ('%2' IN BOOLEAN MODE), 0) * 2.0) +
        ( %3 )
    ) AS score
FROM tickets t
LEFT JOIN ticket_report r ON r.ticket_id = t.ID
WHERE t.is_deleted = 0
  AND t.ID <> ?
  AND (
        %4
        MATCH(t.title, t.description) AGAINST ('%2' IN BOOLEAN MODE)
     OR COALESCE(MATCH(r.report_plain) AGAINST ('%2' IN BOOLEAN MODE), 0)
     OR ( %5 )
  )
ORDER BY score DESC, t.updated_at DESC
LIMIT ?;
)SQL")
                .arg(entityScore)
                .arg(ft)
                .arg(strongScore)
                .arg(entityWhere)
                .arg(strongWhere);

    q.strongTokenCount = maxStrongTokens;
    return q;
}

QString SimilarReportManager::EscapeSqlLiteral(QString s) const { return s.replace('\'', "''"); }
