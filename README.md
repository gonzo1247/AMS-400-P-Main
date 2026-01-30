# P400Basic Database Module

Dieses Projekt stellt eine an TrinityCore angelehnte MySQL/MariaDB-Datenbankschicht bereit, die auf dem [MariaDB Connector/C++](https://mariadb.com/kb/en/mariadb-connector-cpp/) basiert. Ziel ist es, synchrone und asynchrone Datenbankaufgaben robust und threadsicher abzuwickeln.

## Architekturüberblick

* **Konfiguration** (`include/config.h`): Enthält Strukturen für Datenbank-, TLS/SSL- und SSH-Einstellungen, Connection-Pool-Grenzen sowie Diagnoseoptionen.
* **Logging** (`include/logging.h`): Stellt die Makros `LOG_SQL`, `LOG_DEBUG`, `LOG_ERROR`, `LOG_WARNING` bereit.
* **Prepared Statements**
  * Statement-IDs werden über die Enums `IMSPreparedStatement` und `AMSPreparedStatement` in `src/database/Implementation/IMSDatabase.h` bzw. `src/database/Implementation/AMSDatabase.h` vergeben.
  * Die konkreten Statements registrieren sich über `RegisterIMSPreparedStatements()` und `RegisterAMSPreparedStatements()`.
  * `RegisterPreparedStatements()` ruft beide Implementierungen auf und speichert die Metadaten in der globalen Registry (`PreparedStatementRegistry`).
  * Die Registry erlaubt weiterhin den Zugriff per Name oder Alias.
* **Verbindungen & Pools**
  * `DatabaseConnection`: kapselt eine einzelne Verbindung inkl. TLS, Prepared Statements, Transaktionen und Ping.
  * `ConnectionPool`: Kernimplementierung mit getrennten Sync/Async-Pools, Wartungs-Thread, Backpressure über Queue-Limits und Async-Executor.
  * `IMSDatabase` & `AMSDatabase`: je ein eigenständiger Connection-Pool, der dauerhaft die IMS- bzw. AMS-Zugangsdaten nutzt.
  * `DatabaseManager`: interne Fassade, die pro Pool eine eigene `ConnectionPool`-Instanz verwaltet.
* **Abfrageergebnisse**
  * `Field` & `QueryResult`: Ergebnisobjekte, angelehnt an TrinityCore, inkl. Konvertierung von `sql::SQLString` in `std::string`.
* **Async-Unterstützung**
  * `AsyncExecutor`: Single-Worker mit Cancelation-Token für asynchrone Jobs.

## Features

* Synchrone und asynchrone Prepared Statements (wahlweise `std::unique_ptr`, `std::shared_ptr` oder roher Pointer).
* Connection-Pools mit Round-Robin-Akquise, Ping/Health-Checks und automatischem Reconnect.
* Optionales Read/Write-Splitting (Primary/Replica) inkl. Failover-Ping und Lag-Grenzen-Konfiguration.
* Transaktionsunterstützung (`BeginTransaction`, `Commit`, `Rollback`).
* Diagnose-Snapshot (`DiagnosticsSnapshot`) mit Poolgrößen, offenen Verbindungen, Queue-Länge und Prepared-Statement-Statistiken.
* TLS/SSL- und optionale SSH-Tunnel-Konfigurationen (inkl. libssh2-basiertem Local-Forwarding für Datenbankverbindungen).
* Cancelbare Async-Jobs und sauberes Shutdown.

## Abhängigkeiten

* [MariaDB Connector/C++](https://mariadb.com/kb/en/mariadb-connector-cpp/)
* [MariaDB Connector/C](https://mariadb.com/kb/en/mariadb-connector-c/) (transitiv für den C++ Connector)
* Eine funktionsfähige MariaDB- oder MySQL-Instanz

## Nutzung

1. **Prepared Statements registrieren**

   ```cpp
   database::RegisterPreparedStatements();
   ```

2. **Pools konfigurieren**

   ```cpp
   database::PoolConfig imsConfig;
   imsConfig.primary.hostname = "127.0.0.1";
   imsConfig.primary.username = "ims_user";
   imsConfig.primary.password = "ims_secret";
   imsConfig.primary.database = "ims_schema";
   imsConfig.syncLimits = { .minSize = 2, .maxSize = 10, .maxQueueDepth = 2048 };
   imsConfig.asyncLimits = { .minSize = 2, .maxSize = 10, .maxQueueDepth = 2048 };
   imsConfig.maintenance.pingIntervalSeconds = 30;

   database::IMSDatabase::Configure(imsConfig);

   database::PoolConfig amsConfig = imsConfig;
   amsConfig.primary.database = "ams_schema";
   amsConfig.primary.username = "ams_user";
   amsConfig.primary.password = "ams_secret";

   database::AMSDatabase::Configure(amsConfig);
   ```

3. **Sync Statement mit `ConnectionGuard` ausführen**

   Der `ConnectionGuard` stellt sicher, dass Verbindungen bei Verlassen des Scopes automatisch an den entsprechenden Pool
   zurückgegeben werden und optional Warnungen bei zu langer Haltedauer schreiben.

   ```cpp
   // Konstruktor nimmt ConnectionType entgegen und zieht sich selbstständig eine Verbindung
   database::ConnectionGuardAMS guard(
       database::ConnectionType::Sync,
       /*preferReplica=*/false,
       "profile-select"  // optionales Tag für Logmeldungen
   );

   auto statement = guard->GetPreparedStatement(database::Implementation::AMSPreparedStatement::ACCOUNT_SEL_PROFILE);
   statement->SetUInt64(0, 42);
   auto result = guard->ExecutePreparedSelect(*statement);
   if (result.IsValid())
   {
       do
       {
           database::Field* fields = result.Fetch();
           if (!fields)
               break;

           auto accountId = fields[0].Get<std::uint64_t>();
           auto profileKey = fields[1].ToString();
           auto profileValue = fields[2].ToString();
       } while (result.NextRow());
   }
   // guard geht am Scope-Ende automatisch außer Kraft und gibt die Verbindung frei
   ```

   Alternativ lässt sich eine bereits manuell entnommene Verbindung übergeben:

   ```cpp
   auto connection = database::IMSDatabase::GetConnection(database::ConnectionType::Async, /*preferReplica=*/true);
   database::ConnectionGuardIMS guard(std::move(connection), "ims-replica-job");

   guard->Execute("SELECT 1");
   ```

4. **Async Job einreichen**

   ```cpp
   auto token = database::IMSDatabase::ExecuteAsync(
       database::ConnectionType::Async,
       [](std::shared_ptr<database::DatabaseConnection> asyncConnection)
       {
           auto stmt = asyncConnection->GetPreparedStatement(database::Implementation::AMSPreparedStatement::ACCOUNT_UPD_PROFILE);
           stmt->SetString(0, "new value");
           stmt->SetUInt64(1, 42);
           stmt->SetString(2, "nickname");
           asyncConnection->ExecutePreparedModification(*stmt);
       });
   // token.Cancel(); // optionaler Abbruch
   ```

5. **Diagnose abrufen**

   ```cpp
   auto stats = database::IMSDatabase::GetDiagnostics();
   LOG_DEBUG("Sync available: " + std::to_string(stats.syncAvailable));
   ```

## Hinweise

* Für SSH-Verbindungen wird [libssh2](https://www.libssh2.org/) genutzt; stellen Sie sicher, dass die Bibliothek beim Kompilieren/Linken verfügbar ist.
* Für produktive Nutzung sollten zusätzliche Fehlerbehandlungen, Metriken und Tests ergänzt werden.
* Der Wartungs-Thread nutzt aktuell ein SQL-Ping (`SELECT 1`); dieser kann projektspezifisch ersetzt werden.

