#pragma once

#include <chrono>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include "DatabaseTypes.h"
#include "Databases.h"

namespace database
{

namespace detail
{

template <typename Database>
class ConnectionGuard
{
public:
    using Clock = std::chrono::steady_clock;

    explicit ConnectionGuard(std::shared_ptr<DatabaseConnection> connection, std::string tag = {}, std::chrono::milliseconds slowThreshold = std::chrono::milliseconds(1500))
        : connection_(std::move(connection)),
          tag_(std::move(tag)),
          slowThreshold_(slowThreshold),
          startTime_(Clock::now())
    {
    }

    ConnectionGuard(ConnectionType type, bool preferReplica = false, std::string tag = {}, std::chrono::milliseconds slowThreshold = std::chrono::milliseconds(1500))
        : ConnectionGuard(Database::GetConnection(type, preferReplica), std::move(tag), slowThreshold) { }

    ~ConnectionGuard()
    {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - startTime_);
        if (connection_ && elapsed > slowThreshold_)
        {
            std::ostringstream oss;
            oss << "[POOL] SLOW lease tag='" << tag_ << "' held=" << elapsed.count() << "ms";
            oss << " ptr=" << static_cast<const void*>(connection_.get());
            oss << " type="
                << (connection_->GetConnectionType() == ConnectionType::Sync ? "sync" : "async");
            oss << (connection_->IsReplica() ? " replica" : " primary");
            LOG_WARNING(oss.str());
        }

        if (connection_)
        {
            Database::ReturnConnection(connection_);
        }
    }

    ConnectionGuard(const ConnectionGuard&) = delete;
    ConnectionGuard& operator=(const ConnectionGuard&) = delete;
    ConnectionGuard(ConnectionGuard&&) = delete;
    ConnectionGuard& operator=(ConnectionGuard&&) = delete;

    std::shared_ptr<DatabaseConnection> Get() const { return connection_; }
    DatabaseConnection* operator->() const { return connection_.get(); }
    DatabaseConnection& operator*() const { return *connection_; }
    explicit operator bool() const { return static_cast<bool>(connection_); }

    std::shared_ptr<DatabaseConnection> Release()
    {
        return std::exchange(connection_, nullptr);
    }

private:
    std::shared_ptr<DatabaseConnection> connection_;
    std::string tag_;
    std::chrono::milliseconds slowThreshold_;
    Clock::time_point startTime_;
};

} // namespace detail

using ConnectionGuardIMS = detail::ConnectionGuard<IMSDatabase>;
using ConnectionGuardAMS = detail::ConnectionGuard<AMSDatabase>;

} // namespace database

