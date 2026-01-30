#pragma once

#include <QAbstractTableModel>
#include <vector>

struct ArticleDatabase;

class ArticleDatabaseSearchModel final : public QAbstractTableModel
{
    Q_OBJECT

   public:
    enum class Column : int
    {
        ArticleId = 0,
        ArticleName,
        ArticleNumber,
        Manufacturer,
        Supplier,

        Count
    };

    explicit ArticleDatabaseSearchModel(QObject* parent = nullptr);

    void SetData(std::vector<ArticleDatabase> rows);
    const ArticleDatabase& GetRow(int row) const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;

   signals:
    void requestRemove(std::uint64_t rowId);

   private:
    std::vector<ArticleDatabase> _rows;
};
