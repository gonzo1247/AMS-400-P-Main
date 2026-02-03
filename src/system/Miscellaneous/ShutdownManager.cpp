#include "pch.h"
#include "ShutdownManager.h"

#include <QApplication>
#include <QByteArray>
#include <QtGlobal>
#include <QThread>
#include <QThreadPool>
#include <QTimer>
#include <QString>

#include <atomic>
#include <cctype>
#include <cstdlib>
#include <thread>
#include <vector>

#include "Databases.h"
#include "Logger.h"
#include "LoggerDefines.h"

#ifdef _WIN32
#include <tlhelp32.h>
#include <windows.h>
#endif

namespace
{

    std::string ToLowerString(std::string value)
    {
        for (auto& ch : value)
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        return value;
    }

    std::string TrimString(const std::string& value)
    {
        const auto first = value.find_first_not_of(" \t\r\n");
        if (first == std::string::npos)
            return {};
        const auto last = value.find_last_not_of(" \t\r\n");
        return value.substr(first, last - first + 1);
    }

    std::string ToUtf8(const wchar_t* value)
    {
#ifdef _WIN32
        if (!value)
            return {};
        const int len = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
        if (len <= 0)
            return {};
        std::string output(len - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, value, -1, output.data(), len, nullptr, nullptr);
        return output;
#else
        (void)value;
        return {};
#endif
    }

}  // namespace

ShutdownManager& ShutdownManager::Instance()
{
    static ShutdownManager instance;
    return instance;
}

ShutdownManager::ShutdownManager() : processStart_(std::chrono::steady_clock::now()), shutdownStart_(processStart_) {}

void ShutdownManager::Initialize(QApplication* application)
{
    if (initialized_.exchange(true))
        return;

    application_ = application;
    processStart_ = std::chrono::steady_clock::now();
    shutdownStart_ = processStart_;

    if (application_)
    {
        QObject::connect(application_, &QCoreApplication::aboutToQuit, [this]() { OnAboutToQuit(); });
    }
}

void ShutdownManager::RequestShutdown(const std::string& reason)
{
    bool expected = false;
    if (!shutdownRequested_.compare_exchange_strong(expected, true))
        return;

    shutdownStart_ = std::chrono::steady_clock::now();

    LOG_MISC("Shutdown requested: reason={}", reason);
    LOG_MISC("Shutdown force quit enabled: {}", ShouldForceQuit() ? "true" : "false");

    DumpDiagnostics("Shutdown start");
    StartWatchdogs();
    StopEverything();

    if (application_)
        application_->quit();
}

void ShutdownManager::OnAboutToQuit()
{
    LOG_MISC("QApplication aboutToQuit signal received.");
}

void ShutdownManager::OnEventLoopExited(int exitCode)
{
    eventLoopExited_.store(true);
    const auto now = std::chrono::steady_clock::now();
    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - shutdownStart_).count();
    LOG_MISC("QApplication event loop exited with code {} after {} ms.", exitCode, elapsedMs);
    DumpDiagnostics("Shutdown end");
}

void ShutdownManager::OnMainReturning(int exitCode)
{
    mainReturned_.store(true);
    const auto now = std::chrono::steady_clock::now();
    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - shutdownStart_).count();
    LOG_MISC("main() returning with code {} after {} ms from shutdown start.", exitCode, elapsedMs);
}

void ShutdownManager::OnProcessExit()
{
    const auto now = std::chrono::steady_clock::now();
    const auto sinceStartMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - processStart_).count();
    const auto sinceShutdownMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - shutdownStart_).count();
    LOG_MISC("Process exit reached after {} ms ({} ms since shutdown start).", sinceStartMs, sinceShutdownMs);
    LOG_MISC("PROCESS_EXIT_REACHED");
}

void ShutdownManager::StopEverything()
{
    if (!application_)
        return;

    const auto timers = application_->findChildren<QTimer*>();
    std::size_t stoppedTimers = 0;
    for (auto* timer : timers)
    {
        if (timer && timer->isActive())
        {
            timer->stop();
            ++stoppedTimers;
        }
    }
    LOG_MISC("Shutdown: stopped {} active QTimers ({} total).", stoppedTimers, timers.size());

    auto* pool = QThreadPool::globalInstance();
    if (pool)
    {
        pool->clear();
        const bool done = pool->waitForDone(2000);
        LOG_MISC("Shutdown: QThreadPool active={} max={} done={}.", pool->activeThreadCount(), pool->maxThreadCount(),
                 done ? "true" : "false");
    }

    const auto threads = application_->findChildren<QThread*>();
    for (auto* thread : threads)
    {
        if (!thread || thread == QThread::currentThread())
            continue;

        if (thread->isRunning())
        {
            thread->requestInterruption();
            thread->quit();
        }
    }

    for (auto* thread : threads)
    {
        if (!thread || thread == QThread::currentThread())
            continue;

        if (thread->isRunning())
        {
            const bool stopped = thread->wait(1500);
            if (!stopped)
            {
                LOG_WARNING("Shutdown: QThread still running after wait: ptr={}.", static_cast<void*>(thread));
            }
        }
    }

    IMSDatabase::Shutdown();
    AMSDatabase::Shutdown();
}

void ShutdownManager::DumpDiagnostics(const std::string& context)
{
    const auto now = std::chrono::steady_clock::now();
    const auto sinceStartMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - processStart_).count();
    const auto sinceShutdownMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - shutdownStart_).count();

    LOG_MISC("Shutdown diagnostics: {} ({} ms since start, {} ms since shutdown start).", context, sinceStartMs,
             sinceShutdownMs);

    if (application_)
        LOG_MISC("Shutdown diagnostics: quitOnLastWindowClosed={}.", application_->quitOnLastWindowClosed() ? "true" : "false");

    LogTimerSnapshot();
    LogThreadPoolSnapshot();
    LogQtThreadSnapshot();
    LogDatabaseSnapshot();
    LogThreadSnapshot(context);
}

void ShutdownManager::LogTimerSnapshot()
{
    if (!application_)
        return;

    const auto timers = application_->findChildren<QTimer*>();
    std::size_t active = 0;
    for (auto* timer : timers)
    {
        if (timer && timer->isActive())
            ++active;
    }

    LOG_MISC("Shutdown diagnostics: QTimer total={} active={}.", timers.size(), active);

    std::size_t logged = 0;
    for (auto* timer : timers)
    {
        if (!timer || !timer->isActive())
            continue;
        if (logged >= 10)
            break;
        const QString name = timer->objectName();
        LOG_MISC("Shutdown diagnostics: active timer name='{}' intervalMs={} ptr={}.",
                 name.isEmpty() ? "<unnamed>" : name.toStdString(), timer->interval(), static_cast<void*>(timer));
        ++logged;
    }
}

void ShutdownManager::LogQtThreadSnapshot()
{
    if (!application_)
        return;

    const auto threads = application_->findChildren<QThread*>();
    LOG_MISC("Shutdown diagnostics: QThread objects={}.", threads.size());

    std::size_t logged = 0;
    for (auto* thread : threads)
    {
        if (!thread)
            continue;
        if (logged >= 10)
            break;
        const QString name = thread->objectName();
        LOG_MISC("Shutdown diagnostics: QThread name='{}' running={} ptr={}.",
                 name.isEmpty() ? "<unnamed>" : name.toStdString(), thread->isRunning() ? "true" : "false",
                 static_cast<void*>(thread));
        ++logged;
    }
}

void ShutdownManager::LogThreadPoolSnapshot()
{
    auto* pool = QThreadPool::globalInstance();
    if (!pool)
        return;

    LOG_MISC("Shutdown diagnostics: QThreadPool active={} max={} expiryMs={}.", pool->activeThreadCount(),
             pool->maxThreadCount(), pool->expiryTimeout());
}

void ShutdownManager::LogDatabaseSnapshot()
{
    const auto imsDiag = IMSDatabase::GetDiagnostics();
    const auto amsDiag = AMSDatabase::GetDiagnostics();

    LOG_MISC("Shutdown diagnostics: IMS db syncPool={} asyncPool={} replicaSyncPool={} replicaAsyncPool={}.",
             imsDiag.syncPoolSize, imsDiag.asyncPoolSize, imsDiag.replicaSyncPoolSize, imsDiag.replicaAsyncPoolSize);

    LOG_MISC("Shutdown diagnostics: IMS db syncAvail={} asyncAvail={} replicaSyncAvail={} replicaAsyncAvail={} queuedJobs={}.",
        imsDiag.syncAvailable, imsDiag.asyncAvailable, imsDiag.replicaSyncAvailable, imsDiag.replicaAsyncAvailable, imsDiag.queuedJobs);

    LOG_MISC("Shutdown diagnostics: AMS db syncPool={} asyncPool={} replicaSyncPool={} replicaAsyncPool={}.",
             amsDiag.syncPoolSize, amsDiag.asyncPoolSize, amsDiag.replicaSyncPoolSize, amsDiag.replicaAsyncPoolSize);

    LOG_MISC("Shutdown diagnostics: AMS db syncAvail={} asyncAvail={} replicaSyncAvail={} replicaAsyncAvail={} queuedJobs={}.",
        amsDiag.syncAvailable, amsDiag.asyncAvailable, amsDiag.replicaSyncAvailable, amsDiag.replicaAsyncAvailable, amsDiag.queuedJobs);
}

void ShutdownManager::LogThreadSnapshot(const std::string& context)
{
#ifdef _WIN32
    const DWORD currentPid = GetCurrentProcessId();
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        LOG_WARNING("Shutdown diagnostics: thread snapshot failed for {}.", context);
        return;
    }

    THREADENTRY32 entry{};
    entry.dwSize = sizeof(THREADENTRY32);
    std::size_t count = 0;
    std::vector<std::string> names;
    if (Thread32First(snapshot, &entry))
    {
        do
        {
            if (entry.th32OwnerProcessID != currentPid)
                continue;

            ++count;
            HANDLE threadHandle = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, entry.th32ThreadID);
            if (threadHandle)
            {
                using GetThreadDescriptionFn = HRESULT(WINAPI*)(HANDLE, PWSTR*);
                auto* fn = reinterpret_cast<GetThreadDescriptionFn>(
                    GetProcAddress(GetModuleHandleW(L"Kernel32.dll"), "GetThreadDescription"));
                if (fn)
                {
                    PWSTR description = nullptr;
                    if (SUCCEEDED(fn(threadHandle, &description)) && description)
                    {
                        names.emplace_back(ToUtf8(description));
                        LocalFree(description);
                    }
                }
                CloseHandle(threadHandle);
            }
        } while (Thread32Next(snapshot, &entry));
    }
    CloseHandle(snapshot);

    LOG_MISC("Shutdown diagnostics: OS threads count={} for {}.", count, context);
    std::size_t logged = 0;
    for (const auto& name : names)
    {
        if (logged >= 10)
            break;
        if (!name.empty())
        {
            LOG_MISC("Shutdown diagnostics: OS thread name='{}'.", name);
            ++logged;
        }
    }
#else
    LOG_MISC("Shutdown diagnostics: OS thread snapshot not supported for {}.", context);
#endif
}

void ShutdownManager::StartWatchdogs()
{
    if (!application_)
        return;

    QTimer::singleShot(static_cast<int>(diagnosticsDelay_.count()), application_,
                       [this]()
                       {
                           if (!eventLoopExited_.load())
                               DumpDiagnostics("Shutdown watchdog timeout");
                       });

    std::thread(
        [this]()
        {
            std::this_thread::sleep_for(forceQuitDelay_);
            if (eventLoopExited_.load())
                return;

            if (!ShouldForceQuit())
                return;

            DumpDiagnostics("Shutdown forced quit");
            OnProcessExit();
            std::quick_exit(EXIT_FAILURE);
        })
        .detach();
}

bool ShutdownManager::ShouldForceQuit() const
{
    const QByteArray value = qgetenv("AMS_FORCE_QUIT_ON_SHUTDOWN");
    if (value.isEmpty())
        return false;

    std::string raw = value.toStdString();
    raw = TrimString(ToLowerString(raw));
    return raw == "1" || raw == "true" || raw == "yes" || raw == "on";
}
