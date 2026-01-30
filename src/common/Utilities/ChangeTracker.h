#pragma once
#include <QHash>
#include <QObject>
#include <QVariant>
#include <QVector>

enum class MachineField : std::uint8_t
{
    Name,
    Number,
    ManufacturerNumber,
    CostUnitId,
    MachineTypeId,
    MachineLineId,
    ManufacturerId,
    RoomId,
    Location,
    Info,
    Max
};

struct FieldChange
{
    MachineField field;
    QVariant oldValue;
    QVariant newValue;
};

template <typename TEnum>
class ChangeTracker
{
   public:
    void BeginSnapshot()
    {
        _snapshot = _current;
        _dirtyFields.clear();
    }

    void Clear()
    {
        _snapshot.clear();
        _current.clear();
        _dirtyFields.clear();
        _suspended = false;
    }

    bool IsDirty() const { return !_dirtyFields.isEmpty(); }

    QSet<TEnum> GetDirtyFields() const { return _dirtyFields; }

    bool IsSuspended() const { return _suspended; }

    void SetValue(TEnum field, const QVariant& value)
    {
        if (_suspended)
            return;

        SetValueForced(field, value);
        UpdateDirtyState(field);
    }

    void SetValueForced(TEnum field, const QVariant& value) { _current[field] = value; }

    class SuspendGuard
    {
       public:
        explicit SuspendGuard(ChangeTracker& tracker) : _tracker(tracker) { _tracker._suspended = true; }

        ~SuspendGuard() { _tracker._suspended = false; }

       private:
        ChangeTracker& _tracker;
    };

    void RecomputeDirty()
    {
        _dirtyFields.clear();

        for (auto it = _current.constBegin(); it != _current.constEnd(); ++it)
        {
            const TEnum field = it.key();

            if (!_snapshot.contains(field))
                continue;

            if (_current.value(field) != _snapshot.value(field))
                _dirtyFields.insert(field);
        }
    }

    QVariant GetSnapshotValue(TEnum field) const { return _snapshot.value(field); }

    QVariant GetCurrentValue(TEnum field) const { return _current.value(field); }

   private:
    void UpdateDirtyState(TEnum field)
    {
        if (!_snapshot.contains(field))
            return;

        if (_current.value(field) == _snapshot.value(field))
            _dirtyFields.remove(field);
        else
            _dirtyFields.insert(field);
    }

   private:
    QHash<TEnum, QVariant> _snapshot;
    QHash<TEnum, QVariant> _current;
    QSet<TEnum> _dirtyFields;
    bool _suspended = false;
};
