#pragma once

#include <atomic>
#include <chrono>
#include <string>

class QApplication;

class ShutdownManager
{
public:
    static ShutdownManager& Instance();

    void Initialize(QApplication* application);
    void RequestShutdown(const std::string& reason);
    void OnAboutToQuit();
    void OnEventLoopExited(int exitCode);
    void OnMainReturning(int exitCode);
    void OnProcessExit();

private:
    ShutdownManager();

    void StopEverything();
    void DumpDiagnostics(const std::string& context);
    void LogThreadSnapshot(const std::string& context);
    void LogTimerSnapshot();
    void LogQtThreadSnapshot();
    void LogThreadPoolSnapshot();
    void LogDatabaseSnapshot();
    void StartWatchdogs();
    bool ShouldForceQuit() const;

    std::chrono::steady_clock::time_point processStart_;
    std::chrono::steady_clock::time_point shutdownStart_;

    QApplication* application_{nullptr};
    std::atomic<bool> initialized_{false};
    std::atomic<bool> shutdownRequested_{false};
    std::atomic<bool> eventLoopExited_{false};
    std::atomic<bool> mainReturned_{false};

    std::chrono::milliseconds diagnosticsDelay_{std::chrono::milliseconds(5000)};
    std::chrono::milliseconds forceQuitDelay_{std::chrono::milliseconds(10000)};
};
