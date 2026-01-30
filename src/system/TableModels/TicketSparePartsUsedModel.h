#pragma once

#include <QAbstractTableModel>
#include <vector>

struct SparePartsTable;
struct TicketSparePartUsedInformation;

class TicketSparePartsUsedModel final : public QAbstractTableModel
{
    Q_OBJECT

   public:
    enum class Column : int
    {
        ArticleId = 0,
        Quantity,
        Unit,
        Note,
        CreatedBy,
        CreatedAt,
        Action,

        Count
    };

    explicit TicketSparePartsUsedModel(QObject* parent = nullptr);

    void SetData(std::vector<SparePartsTable> rows);
    const SparePartsTable& GetRow(int row) const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    bool RemoveById(std::uint64_t rowId);
    void AppendRow(SparePartsTable row);

   signals:
    void requestRemove(std::uint64_t rowId);

   private:
    std::vector<SparePartsTable> _rows;
};
