#pragma once

#include <functional>
#include <utility>

#include "DatabaseManager.h"

using namespace database;
using namespace database::Implementation;

namespace database
{

namespace detail
{
    template <typename Tag>
    class NamedDatabase
    {
    public:
        static void Configure(const PoolConfig& config)
        {
            Instance().Configure(config);
        }

        static std::shared_ptr<DatabaseConnection> GetConnection(ConnectionType type, bool preferReplica = false)
        {
            return Instance().GetConnection(type, preferReplica);
        }

        static void ReturnConnection(const std::shared_ptr<DatabaseConnection>& connection)
        {
            Instance().ReturnConnection(connection);
        }

        static CancellationToken ExecuteAsync(ConnectionType type, std::function<void(std::shared_ptr<DatabaseConnection>)> task, bool preferReplica = false)
        {
            return Instance().ExecuteAsync(type, std::move(task), preferReplica);
        }

        static DiagnosticsSnapshot GetDiagnostics()
        {
            return Instance().GetDiagnostics();
        }

        static void Shutdown()
        {
            Instance().Shutdown();
        }

    private:
        static DatabaseManager& Instance()
        {
            static DatabaseManager manager;
            return manager;
        }
    };
} // namespace detail

struct IMSDatabaseTag
{
};
using IMSDatabase = detail::NamedDatabase<IMSDatabaseTag>;

struct AMSDatabaseTag
{
};
using AMSDatabase = detail::NamedDatabase<AMSDatabaseTag>;

} // namespace database
