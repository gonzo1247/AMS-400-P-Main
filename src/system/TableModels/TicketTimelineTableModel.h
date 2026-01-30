#pragma once

#include <QAbstractTableModel>
#include <QIcon>
#include <QString>
#include <QColor>

#include <optional>
#include <vector>

#include "ShowTicketManager.h"

class TicketTimelineTableModel : public QAbstractTableModel
{
    Q_OBJECT

   public:
    explicit TicketTimelineTableModel(QObject* parent = nullptr);

    void setEntries(const std::vector<TicketTimelineEntry>& entries);
    void setFilterText(const QString& text);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orient, int role) const override;
    void sort(int column, Qt::SortOrder order) override;

    std::optional<TicketTimelineEntry> entryForRow(int row) const;
    std::optional<TicketTimelineEntry> entryForIndex(const QModelIndex& index) const;

   private:
    void rebuildRows();

    QString typeToString(TicketTimelineType type) const;
    QString summaryForEntry(const TicketTimelineEntry& entry) const;

    // Prepared for later use (icons), currently unused
    QIcon iconForType(TicketTimelineType type) const;

    QColor colorForEntry(const TicketTimelineEntry& entry) const;

    QString actorForEntry(const TicketTimelineEntry& entry) const;

   private:
    QString _filterText{};
    std::vector<TicketTimelineEntry> _allEntries;
    std::vector<TicketTimelineEntry> _entries;
};
