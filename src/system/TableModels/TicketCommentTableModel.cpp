#include "pch.h"
#include "TicketCommentTableModel.h"

#include <algorithm>
#include <ranges>

#include "UserCache.h"

TicketCommentTableModel::TicketCommentTableModel(QObject* parent) : QAbstractTableModel(parent) {}

void TicketCommentTableModel::setData(const std::vector<TicketCommentInformation>& vec)
{
    _allRows = vec;  // copy vector
    rebuildRows();
}

void TicketCommentTableModel::setFilterText(const QString& text)
{
    if (_filterText == text)
        return;

    _filterText = text;
    rebuildRows();
}

void TicketCommentTableModel::setHideDeleted(bool hideDeleted)
{
    if (_hideDeleted == hideDeleted)
        return;

    _hideDeleted = hideDeleted;
    rebuildRows();
}

void TicketCommentTableModel::setShowInternalOnly(bool internalOnly)
{
    if (_showInternalOnly == internalOnly)
        return;

    _showInternalOnly = internalOnly;
    rebuildRows();
}

int TicketCommentTableModel::rowCount(const QModelIndex&) const { return static_cast<int>(_rows.size()); }

int TicketCommentTableModel::columnCount(const QModelIndex&) const { return 7; }

// --- Helper to convert timestamp ---
QString TicketCommentTableModel::formatTimestamp(const SystemTimePoint& tp)
{
    if (tp == SystemTimePoint{})
        return {};

    const auto secs = std::chrono::time_point_cast<std::chrono::seconds>(tp);
    const std::time_t t = std::chrono::system_clock::to_time_t(secs);

    return QDateTime::fromSecsSinceEpoch(static_cast<qint64>(t)).toString("yyyy-MM-dd HH:mm:ss");
}

QVariant TicketCommentTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || static_cast<std::size_t>(index.row()) >= _rows.size())
    {
        return {};
    }

    const auto& row = _rows[index.row()];

    // --- Background color for internal comments ---
    if (role == Qt::BackgroundRole)
    {
        if (row.isInternal)
            return QBrush(QColor(255, 250, 180));  // soft yellow
        if (row.isDeleted)
            return QBrush(QColor(230, 230, 230));  // light gray
    }

    if (role == Qt::DisplayRole)
    {
        switch (index.column())
        {
            case 0:
                return static_cast<qulonglong>(row.id);
            case 1:
                return formatTimestamp(row.createdAt);
            case 2:
                return formatTimestamp(row.updatedAt);
            case 3:
            {
                auto userData = UserCache::instance().GetUserDataByID(row.authorUserID);
                QString name = {};;
                if (userData.has_value())
                    name = QString::fromStdString(userData->GetUserLastName());

                return name;
            }
            case 4:
                return row.isInternal ? "Yes" : "No";
            case 5:
                return row.isDeleted ? "Yes" : "No";
            case 6:
                return QString::fromUtf8(row.message);
        }
    }

    if (role == Qt::CheckStateRole)
    {
        if (index.column() == 4)
            return row.isInternal ? Qt::Checked : Qt::Unchecked;

        if (index.column() == 5)
            return row.isDeleted ? Qt::Checked : Qt::Unchecked;
    }

    return {};
}

QVariant TicketCommentTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    switch (section)
    {
        case 0:
            return tr("ID");
        case 1:
            return tr("Created");
        case 2:
            return tr("Updated");
        case 3:
            return tr("Author");
        case 4:
            return tr("Internal");
        case 5:
            return tr("Deleted");
        case 6:
            return tr("Message");
    }

    return {};
}

void TicketCommentTableModel::sort(int column, Qt::SortOrder order)
{
    if (_allRows.empty())
        return;

    auto cmp = [column, order](const TicketCommentInformation& a, const TicketCommentInformation& b)
    {
        auto compare = [&]() -> int
        {
            switch (column)
            {
                case 0:
                    return (a.id < b.id) ? -1 : (a.id > b.id ? 1 : 0);
                case 1:
                    return (a.createdAt < b.createdAt) ? -1 : (a.createdAt > b.createdAt ? 1 : 0);
                case 2:
                    return (a.updatedAt < b.updatedAt) ? -1 : (a.updatedAt > b.updatedAt ? 1 : 0);
                case 3:
                    return (a.authorUserID < b.authorUserID) ? -1 : (a.authorUserID > b.authorUserID ? 1 : 0);
                case 4:
                    return (a.isInternal == b.isInternal) ? 0 : (a.isInternal ? -1 : 1);
                case 5:
                    return (a.isDeleted == b.isDeleted) ? 0 : (a.isDeleted ? -1 : 1);
                case 6:
                {
                    const auto sa = QString::fromUtf8(a.message);
                    const auto sb = QString::fromUtf8(b.message);
                    return QString::localeAwareCompare(sa, sb);
                }
            }
            return 0;
        };

        int r = compare();
        if (r == 0)
            return false;

        return order == Qt::AscendingOrder ? (r < 0) : (r > 0);
    };

    beginResetModel();
    std::ranges::sort(_allRows, cmp);
    rebuildRows();
    endResetModel();
}

void TicketCommentTableModel::rebuildRows()
{
    beginResetModel();

    _rows.clear();
    _rows.reserve(_allRows.size());

    const QString filter = _filterText.trimmed();
    const bool hasFilter = !filter.isEmpty();

    for (const auto& c : _allRows)
    {
        if (_hideDeleted && c.isDeleted)
            continue;

        if (_showInternalOnly && !c.isInternal)
            continue;

        if (hasFilter)
        {
            const QString msg = QString::fromUtf8(c.message);
            const QString author = QString::number(c.authorUserID);

            bool match = msg.contains(filter, Qt::CaseInsensitive) || author.contains(filter, Qt::CaseInsensitive);

            if (!match)
                continue;
        }

        _rows.push_back(c);
    }

    endResetModel();
}

std::optional<TicketCommentInformation> TicketCommentTableModel::getComment(int row) const
{
    if (row < 0 || static_cast<std::size_t>(row) >= _rows.size())
        return std::nullopt;

    return _rows[row];
}
