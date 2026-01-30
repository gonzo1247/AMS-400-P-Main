#pragma once

#include <QAbstractTableModel>
#include <QDateTime>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

class RoomTableModel : public QAbstractTableModel
{
    Q_OBJECT

   public:
    struct Row
    {
        std::uint32_t id = 0;
        std::string code;
        std::string name;
        bool isDeleted = false;
        QDateTime deletedAt;
    };

    struct Provider
    {
        // Unscoped
        std::function<std::vector<Row>(bool includeDeleted)> load;
        std::function<std::uint32_t(const std::string& code, const std::string& name)> insert;
        std::function<bool(std::uint32_t id, const std::string& code, const std::string& name)> update;
        std::function<bool(std::uint32_t id)> softDelete;
        std::function<bool(std::uint32_t id)> restore;               // optional
        std::function<bool(std::uint32_t id)> canDelete;             // optional
        std::function<QString(std::uint32_t id)> deleteBlockReason;  // optional

        // Scoped (optional, e.g. by location)
        std::function<std::uint32_t()> scopeId;

        std::function<std::vector<Row>(std::uint32_t scopeId, bool includeDeleted)> loadScoped;
        std::function<std::uint32_t(std::uint32_t scopeId, const std::string& code, const std::string& name)>
            insertScoped;
        std::function<bool(std::uint32_t scopeId, std::uint32_t id, const std::string& code, const std::string& name)>
            updateScoped;
        std::function<bool(std::uint32_t scopeId, std::uint32_t id)> softDeleteScoped;
        std::function<bool(std::uint32_t scopeId, std::uint32_t id)> restoreScoped;  // optional
    };

    enum class Column : int
    {
        Id = 0,
        Code = 1,
        Name = 2,
        Max
    };

    explicit RoomTableModel(Provider provider, QObject* parent = nullptr);

    void setIncludeDeleted(bool includeDeleted);
    bool includeDeleted() const;

    bool reload();

    std::uint32_t addNew(const QString& defaultCode = QStringLiteral("000"),
                         const QString& defaultName = QStringLiteral("New"));

    bool softDeleteRow(int row);
    bool restoreRow(int row);

    const Row* rowAt(int row) const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

   private:
    Provider _provider;
    std::vector<Row> _rows;
    bool _includeDeleted = false;

    bool isValidRow(int row) const;
    std::uint32_t currentScopeId() const;
};
