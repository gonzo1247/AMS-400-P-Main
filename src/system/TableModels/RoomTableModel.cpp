#include "pch.h"
#include "RoomTableModel.h"

#include <QBrush>
#include <QColor>

RoomTableModel::RoomTableModel(Provider provider, QObject* parent)
    : QAbstractTableModel(parent), _provider(std::move(provider))
{
}

void RoomTableModel::setIncludeDeleted(bool includeDeleted)
{
    if (_includeDeleted == includeDeleted)
        return;

    _includeDeleted = includeDeleted;
    reload();
}

bool RoomTableModel::includeDeleted() const { return _includeDeleted; }

std::uint32_t RoomTableModel::currentScopeId() const
{
    if (_provider.scopeId)
        return _provider.scopeId();

    return 0;
}

bool RoomTableModel::reload()
{
    beginResetModel();
    _rows.clear();

    if (_provider.scopeId && _provider.loadScoped)
    {
        _rows = _provider.loadScoped(currentScopeId(), _includeDeleted);
        endResetModel();
        return true;
    }

    if (_provider.load)
    {
        _rows = _provider.load(_includeDeleted);
        endResetModel();
        return true;
    }

    endResetModel();
    return false;
}

std::uint32_t RoomTableModel::addNew(const QString& defaultCode, const QString& defaultName)
{
    const std::string code = defaultCode.trimmed().toStdString();
    const std::string name = defaultName.trimmed().toStdString();

    if (code.empty() || name.empty())
        return 0;

    std::uint32_t newId = 0;

    if (_provider.scopeId && _provider.insertScoped)
    {
        newId = _provider.insertScoped(currentScopeId(), code, name);
        reload();
        return newId;
    }

    if (!_provider.insert)
        return 0;

    newId = _provider.insert(code, name);
    reload();
    return newId;
}

bool RoomTableModel::softDeleteRow(int row)
{
    if (!isValidRow(row))
        return false;

    const Row& r = _rows[static_cast<std::size_t>(row)];

    if (!r.isDeleted)
    {
        if (_provider.canDelete && !_provider.canDelete(r.id))
            return false;

        bool ok = false;

        if (_provider.scopeId && _provider.softDeleteScoped)
            ok = _provider.softDeleteScoped(currentScopeId(), r.id);
        else if (_provider.softDelete)
            ok = _provider.softDelete(r.id);
        else
            return false;

        if (!ok)
            return false;
    }

    return reload();
}

bool RoomTableModel::restoreRow(int row)
{
    if (!isValidRow(row))
        return false;

    const Row& r = _rows[static_cast<std::size_t>(row)];

    if (!r.isDeleted)
        return reload();

    bool ok = false;

    if (_provider.scopeId && _provider.restoreScoped)
        ok = _provider.restoreScoped(currentScopeId(), r.id);
    else if (_provider.restore)
        ok = _provider.restore(r.id);
    else
        return false;

    if (!ok)
        return false;

    return reload();
}

const RoomTableModel::Row* RoomTableModel::rowAt(int row) const
{
    if (!isValidRow(row))
        return nullptr;

    return &_rows[static_cast<std::size_t>(row)];
}

int RoomTableModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return static_cast<int>(_rows.size());
}

int RoomTableModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return static_cast<int>(Column::Max);
}

QVariant RoomTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return {};

    const int r = index.row();
    if (!isValidRow(r))
        return {};

    const Row& row = _rows[static_cast<std::size_t>(r)];
    const auto col = static_cast<Column>(index.column());

    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch (col)
        {
            case Column::Id:
                return static_cast<qulonglong>(row.id);
            case Column::Code:
                return QString::fromStdString(row.code);
            case Column::Name:
                return QString::fromStdString(row.name);
            default:
                return {};
        }
    }

    if (role == Qt::TextAlignmentRole)
    {
        switch (col)
        {
            case Column::Id:
                return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
            default:
                return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
        }
    }

    if (role == Qt::ForegroundRole && row.isDeleted)
        return QBrush(QColor(150, 150, 150));

    if (role == Qt::FontRole && row.isDeleted)
    {
        QFont f;
        f.setStrikeOut(true);
        return f;
    }

    if (role == Qt::ToolTipRole && row.isDeleted && _provider.deleteBlockReason)
    {
        const QString reason = _provider.deleteBlockReason(row.id);
        if (!reason.isEmpty())
            return reason;
    }

    return {};
}

QVariant RoomTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    const auto col = static_cast<Column>(section);

    switch (col)
    {
        case Column::Id:
            return QStringLiteral("ID");
        case Column::Code:
            return QStringLiteral("Code");
        case Column::Name:
            return QStringLiteral("Name");
        default:
            return {};
    }
}

Qt::ItemFlags RoomTableModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    const int r = index.row();
    if (!isValidRow(r))
        return Qt::NoItemFlags;

    const Row& row = _rows[static_cast<std::size_t>(r)];
    const auto col = static_cast<Column>(index.column());

    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    if (!row.isDeleted && (col == Column::Code || col == Column::Name))
        f |= Qt::ItemIsEditable;

    return f;
}

bool RoomTableModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    const int r = index.row();
    if (!isValidRow(r))
        return false;

    Row& row = _rows[static_cast<std::size_t>(r)];
    if (row.isDeleted)
        return false;

    const auto col = static_cast<Column>(index.column());
    if (col != Column::Code && col != Column::Name)
        return false;

    const QString s = value.toString().trimmed();
    if (s.isEmpty())
        return false;

    std::string newCode = row.code;
    std::string newName = row.name;

    if (col == Column::Code)
        newCode = s.toStdString();
    else
        newName = s.toStdString();

    if (newCode.empty() || newName.empty())
        return false;

    if (newCode == row.code && newName == row.name)
        return true;

    bool ok = false;

    if (_provider.scopeId && _provider.updateScoped)
        ok = _provider.updateScoped(currentScopeId(), row.id, newCode, newName);
    else if (_provider.update)
        ok = _provider.update(row.id, newCode, newName);
    else
        return false;

    if (!ok)
        return false;

    row.code = std::move(newCode);
    row.name = std::move(newName);

    emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
    return true;
}

bool RoomTableModel::isValidRow(int row) const { return row >= 0 && row < static_cast<int>(_rows.size()); }
