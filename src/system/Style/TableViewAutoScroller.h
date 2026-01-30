#pragma once

#include <QElapsedTimer>
#include <QMetaObject>
#include <QPersistentModelIndex>
#include <QPointer>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QScrollBar;
class QTableView;
QT_END_NAMESPACE

class TableViewAutoScroller : public QObject
{
    Q_OBJECT

   public:
    explicit TableViewAutoScroller(QTableView* view = nullptr, QObject* parent = nullptr);
    ~TableViewAutoScroller() override;

    void SetView(QTableView* view);
    QTableView* GetView() const;

    void Start();
    void Stop();
    bool IsRunning() const;

    void BeginTicketTableUpdate();
    void EndTicketTableUpdate();

    void HookModelForScrollPreserve();

    void SetSpeedDownPxPerSec(double px_per_sec);
    void SetSpeedUpPxPerSec(double px_per_sec);
    void SetPauseTopMs(int ms);
    void SetPauseBottomMs(int ms);
    void SetTickMs(int ms);

    double GetSpeedDownPxPerSec() const;
    double GetSpeedUpPxPerSec() const;
    int GetPauseTopMs() const;
    int GetPauseBottomMs() const;
    int GetTickMs() const;

   private slots:
    void AutoScrollTick();

   private:
    enum class AutoScrollState
    {
        PauseTop,
        Down,
        PauseBottom,
        Up
    };

    struct ScrollSnapshot
    {
        QPersistentModelIndex topIndex;
        int yOffset = 0;
        int sbValue = 0;
    };

   private:
    QScrollBar* GetScrollBar() const;
    QAbstractItemModel* GetModel() const;

    ScrollSnapshot CaptureSnapshot() const;
    void RestoreSnapshot(const ScrollSnapshot& s) const;
    void RestoreSnapshotQueued(const ScrollSnapshot& s) const;

    void ClearModelConnections();
    void HookModel(QAbstractItemModel* model);

   private:
    QPointer<QTableView> _view;

    QTimer _timer;
    QElapsedTimer _elapsed;

    bool _pausedForUpdate = false;

    AutoScrollState _state = AutoScrollState::PauseTop;
    int _pauseRemainingMs = 0;

    double _speedDownPxPerSec = 1.0;
    double _speedUpPxPerSec = 999.0;
    int _pauseTopMs = 5000;
    int _pauseBottomMs = 5000;
    int _tickMs = 25;

    ScrollSnapshot _snapshot;

    QPointer<QAbstractItemModel> _hookedModel;
    QList<QMetaObject::Connection> _modelConnections;
};
