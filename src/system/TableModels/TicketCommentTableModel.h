#pragma once

#include <QAbstractTableModel>
#include <QString>
#include <string>
#include <vector>

#include "DatabaseDefines.h"
#include "Duration.h"

class TicketCommentTableModel : public QAbstractTableModel
{
    Q_OBJECT

   public:
    explicit TicketCommentTableModel(QObject* parent = nullptr);

    void setData(const std::vector<TicketCommentInformation>& vec);
    void setFilterText(const QString& text);
    void setHideDeleted(bool hideDeleted);
    void setShowInternalOnly(bool internalOnly);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void sort(int column, Qt::SortOrder order) override;

    std::optional<TicketCommentInformation> getComment(int row) const;

   private:
    std::vector<TicketCommentInformation> _allRows;
    std::vector<TicketCommentInformation> _rows;

    QString _filterText{};
    bool _hideDeleted{true};
    bool _showInternalOnly{false};

    void rebuildRows();
    static QString formatTimestamp(const SystemTimePoint& tp);
};
