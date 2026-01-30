#pragma once

#include <QAbstractItemModel>
#include <QScrollBar>
#include <QTableView>
#include <algorithm>

#include "TableViewAutoScroller.h"
#include "pch.h"

TableViewAutoScroller::TableViewAutoScroller(QTableView* view, QObject* parent) : QObject(parent), _view(view)
{
    _timer.setTimerType(Qt::PreciseTimer);
    connect(&_timer, &QTimer::timeout, this, &TableViewAutoScroller::AutoScrollTick);

    if (_view)
    {
        HookModelForScrollPreserve();
    }
}

TableViewAutoScroller::~TableViewAutoScroller()
{
    Stop();
    ClearModelConnections();
}

void TableViewAutoScroller::SetView(QTableView* view)
{
    if (_view == view)
    {
        return;
    }

    Stop();
    ClearModelConnections();

    _view = view;

    if (_view)
    {
        HookModelForScrollPreserve();
    }
}

QTableView* TableViewAutoScroller::GetView() const { return _view.data(); }

void TableViewAutoScroller::Start()
{
    if (_timer.isActive())
    {
        return;
    }

    if (!_view)
    {
        return;
    }

    _view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    _state = AutoScrollState::PauseTop;
    _pauseRemainingMs = _pauseTopMs;

    _elapsed.restart();
    _timer.start(_tickMs);
}

void TableViewAutoScroller::Stop()
{
    if (!_timer.isActive())
    {
        return;
    }

    _timer.stop();
}

bool TableViewAutoScroller::IsRunning() const { return _timer.isActive(); }

void TableViewAutoScroller::SetSpeedDownPxPerSec(double px_per_sec) {
    _speedDownPxPerSec = std::max(1.0, px_per_sec);
}

void TableViewAutoScroller::SetSpeedUpPxPerSec(double px_per_sec) { _speedUpPxPerSec = std::max(1.0, px_per_sec); }

void TableViewAutoScroller::SetPauseTopMs(int ms) { _pauseTopMs = std::max(0, ms); }

void TableViewAutoScroller::SetPauseBottomMs(int ms) { _pauseBottomMs = std::max(0, ms); }

void TableViewAutoScroller::SetTickMs(int ms)
{
    _tickMs = std::clamp(ms, 5, 200);

    if (_timer.isActive())
    {
        _timer.start(_tickMs);
    }
}

double TableViewAutoScroller::GetSpeedDownPxPerSec() const { return _speedDownPxPerSec; }

double TableViewAutoScroller::GetSpeedUpPxPerSec() const { return _speedUpPxPerSec; }

int TableViewAutoScroller::GetPauseTopMs() const { return _pauseTopMs; }

int TableViewAutoScroller::GetPauseBottomMs() const { return _pauseBottomMs; }

int TableViewAutoScroller::GetTickMs() const { return _tickMs; }

QScrollBar* TableViewAutoScroller::GetScrollBar() const
{
    if (!_view)
    {
        return nullptr;
    }

    return _view->verticalScrollBar();
}

QAbstractItemModel* TableViewAutoScroller::GetModel() const
{
    if (!_view)
    {
        return nullptr;
    }

    return _view->model();
}

TableViewAutoScroller::ScrollSnapshot TableViewAutoScroller::CaptureSnapshot() const
{
    ScrollSnapshot s{};

    if (!_view)
    {
        return s;
    }

    if (auto* sb = GetScrollBar())
    {
        s.sbValue = sb->value();
    }

    const QModelIndex idx = _view->indexAt(QPoint(0, 0));
    if (idx.isValid())
    {
        s.topIndex = QPersistentModelIndex(idx);
        s.yOffset = _view->visualRect(idx).y();
    }

    return s;
}

void TableViewAutoScroller::RestoreSnapshot(const ScrollSnapshot& s) const
{
    if (!_view)
    {
        return;
    }

    auto* sb = GetScrollBar();
    if (!sb)
    {
        return;
    }

    const int min = sb->minimum();
    const int max = sb->maximum();

    if (s.sbValue > max)
    {
        sb->setValue(min);
        _view->scrollToTop();
        return;
    }

    if (s.topIndex.isValid())
    {
        _view->scrollTo(s.topIndex, QAbstractItemView::PositionAtTop);

        const int desired = sb->value() - s.yOffset;
        sb->setValue(std::clamp(desired, min, max));
        return;
    }

    sb->setValue(std::clamp(s.sbValue, min, max));
}

void TableViewAutoScroller::RestoreSnapshotQueued(const ScrollSnapshot& s) const
{
    if (!_view)
    {
        return;
    }

    QTimer::singleShot(0, _view, [this, s]() { RestoreSnapshot(s); });
}

void TableViewAutoScroller::BeginTicketTableUpdate()
{
    if (!_view)
    {
        return;
    }

    _pausedForUpdate = true;
    _snapshot = CaptureSnapshot();

    _view->setUpdatesEnabled(false);
}

void TableViewAutoScroller::EndTicketTableUpdate()
{
    if (!_view)
    {
        return;
    }

    _view->setUpdatesEnabled(true);

    RestoreSnapshotQueued(_snapshot);

    _pausedForUpdate = false;
}

void TableViewAutoScroller::AutoScrollTick()
{
    if (_pausedForUpdate)
    {
        return;
    }

    if (!_view)
    {
        return;
    }

    if (!GetModel())
    {
        return;
    }

    auto* sb = GetScrollBar();
    if (!sb)
    {
        return;
    }

    const int min = sb->minimum();
    const int max = sb->maximum();

    if (max <= min)
    {
        _state = AutoScrollState::PauseTop;
        _pauseRemainingMs = _pauseTopMs;
        _elapsed.restart();
        return;
    }

    const qint64 dt_raw = _elapsed.restart();
    const int dt_ms = static_cast<int>((dt_raw > 0) ? dt_raw : _tickMs);

    if (_state == AutoScrollState::PauseTop || _state == AutoScrollState::PauseBottom)
    {
        _pauseRemainingMs -= dt_ms;
        if (_pauseRemainingMs > 0)
        {
            return;
        }

        _state = (_state == AutoScrollState::PauseTop) ? AutoScrollState::Down : AutoScrollState::Up;
        return;
    }

    const double speed = (_state == AutoScrollState::Down) ? _speedDownPxPerSec : _speedUpPxPerSec;
    const int delta = std::max(1, static_cast<int>(speed * (static_cast<double>(dt_ms) / 1000.0)));

    const int value = sb->value();

    if (_state == AutoScrollState::Down)
    {
        const int next = value + delta;

        if (next >= max)
        {
            sb->setValue(max);
            _view->scrollToBottom();

            _state = AutoScrollState::PauseBottom;
            _pauseRemainingMs = _pauseBottomMs;
            return;
        }

        sb->setValue(next);
        return;
    }

    const int next = value - delta;

    if (next <= min)
    {
        sb->setValue(min);
        _view->scrollToTop();

        _state = AutoScrollState::PauseTop;
        _pauseRemainingMs = _pauseTopMs;
        return;
    }

    sb->setValue(next);
}

void TableViewAutoScroller::ClearModelConnections()
{
    for (const auto& c : _modelConnections)
    {
        disconnect(c);
    }
    _modelConnections.clear();
    _hookedModel.clear();
}

void TableViewAutoScroller::HookModel(QAbstractItemModel* model)
{
    if (!model)
    {
        return;
    }

    _hookedModel = model;

    _modelConnections.push_back(
        connect(model, &QAbstractItemModel::modelAboutToBeReset, this, [this]() { BeginTicketTableUpdate(); }));

    _modelConnections.push_back(
        connect(model, &QAbstractItemModel::modelReset, this, [this]() { EndTicketTableUpdate(); }));

    _modelConnections.push_back(
        connect(model, &QAbstractItemModel::layoutAboutToBeChanged, this, [this]() { BeginTicketTableUpdate(); }));

    _modelConnections.push_back(
        connect(model, &QAbstractItemModel::layoutChanged, this, [this]() { EndTicketTableUpdate(); }));

    auto preserve = [this]()
    {
        BeginTicketTableUpdate();
        EndTicketTableUpdate();
    };

    _modelConnections.push_back(connect(model, &QAbstractItemModel::rowsInserted, this, preserve));
    _modelConnections.push_back(connect(model, &QAbstractItemModel::rowsRemoved, this, preserve));
    _modelConnections.push_back(connect(model, &QAbstractItemModel::rowsMoved, this, preserve));
}

void TableViewAutoScroller::HookModelForScrollPreserve()
{
    if (!_view)
    {
        return;
    }

    auto* model = GetModel();
    if (!model)
    {
        return;
    }

    if (_hookedModel == model)
    {
        return;
    }

    ClearModelConnections();
    HookModel(model);
}
