#include "LookupTableModel.h"

#include <QBrush>
#include <QColor>

#include "pch.h"

LookupTableModel::LookupTableModel(Provider provider, QObject* parent)
    : QAbstractTableModel(parent), _provider(std::move(provider))
{
}

void LookupTableModel::setIncludeDeleted(bool includeDeleted)
{
    if (_includeDeleted == includeDeleted)
        return;

    _includeDeleted = includeDeleted;
    reload();
}

bool LookupTableModel::includeDeleted() const { return _includeDeleted; }

bool LookupTableModel::isScopedProvider() const
{
    return _provider.scopeId && _provider.loadScoped && _provider.insertScoped && _provider.updateNameScoped &&
           _provider.softDeleteScoped;
}

std::uint32_t LookupTableModel::currentScopeId() const
{
    if (_provider.scopeId)
        return _provider.scopeId();

    return 0;
}

bool LookupTableModel::reload()
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

std::uint32_t LookupTableModel::addNew(const QString& defaultName)
{
    const std::string name = defaultName.toStdString();
    std::uint32_t newId = 0;

    if (_provider.scopeId && _provider.insertScoped)
    {
        newId = _provider.insertScoped(currentScopeId(), name);
        reload();
        return newId;
    }

    if (!_provider.insert)
        return 0;

    newId = _provider.insert(name);
    reload();
    return newId;
}

bool LookupTableModel::softDeleteRow(int row)
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

bool LookupTableModel::restoreRow(int row)
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

const LookupTableModel::Row* LookupTableModel::rowAt(int row) const
{
    if (!isValidRow(row))
        return nullptr;

    return &_rows[static_cast<std::size_t>(row)];
}

int LookupTableModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return static_cast<int>(_rows.size());
}

int LookupTableModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return static_cast<int>(Column::Max);
}

QVariant LookupTableModel::data(const QModelIndex& index, int role) const
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

QVariant LookupTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    const auto col = static_cast<Column>(section);

    switch (col)
    {
        case Column::Id:
            return QStringLiteral("ID");
        case Column::Name:
            return QStringLiteral("Name");
        default:
            return {};
    }
}

Qt::ItemFlags LookupTableModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    const int r = index.row();
    if (!isValidRow(r))
        return Qt::NoItemFlags;

    const Row& row = _rows[static_cast<std::size_t>(r)];
    const auto col = static_cast<Column>(index.column());

    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    if (col == Column::Name && !row.isDeleted)
        f |= Qt::ItemIsEditable;

    return f;
}

bool LookupTableModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    const int r = index.row();
    if (!isValidRow(r))
        return false;

    Row& row = _rows[static_cast<std::size_t>(r)];
    if (row.isDeleted)
        return false;

    if (static_cast<Column>(index.column()) != Column::Name)
        return false;

    const QString newNameQt = value.toString().trimmed();
    if (newNameQt.isEmpty())
        return false;

    const std::string newName = newNameQt.toStdString();
    if (newName == row.name)
        return true;

    bool ok = false;

    if (_provider.scopeId && _provider.updateNameScoped)
        ok = _provider.updateNameScoped(currentScopeId(), row.id, newName);
    else if (_provider.updateName)
        ok = _provider.updateName(row.id, newName);
    else
        return false;

    if (!ok)
        return false;

    row.name = newName;
    emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
    return true;
}

bool LookupTableModel::isValidRow(int row) const { return row >= 0 && row < static_cast<int>(_rows.size()); }
