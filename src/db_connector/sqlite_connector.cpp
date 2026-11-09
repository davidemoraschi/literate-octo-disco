#include "db_connector/sqlite_connector.h"

#include "db_connector/bigquery_connector.h"
#include "db_connector/sqlserver_connector.h"

#include <stdexcept>
#include <string>
#include <utility>

#ifdef SQLBOT_ENABLE_SQLITE3
#include <sqlite3.h>
#endif

namespace sqlbot {

void SQLiteConnector::Connect(const std::string& connection_string) {
    if (connection_string.empty()) {
        throw std::runtime_error("SQLite connection string is empty. Expected a database file path.");
    }
    db_path_ = connection_string;
}

QueryResult SQLiteConnector::Execute(const std::string& sql) {
    if (db_path_.empty()) {
        throw std::runtime_error("SQLite database is not connected.");
    }
#ifdef SQLBOT_ENABLE_SQLITE3
    sqlite3* db = nullptr;
    if (sqlite3_open_v2(db_path_.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) {
        std::string err = sqlite3_errmsg(db);
        if (db != nullptr) {
            sqlite3_close(db);
        }
        throw std::runtime_error("sqlite3_open_v2 failed: " + err);
    }

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), static_cast<int>(sql.size()), &stmt, nullptr) != SQLITE_OK) {
        std::string err = sqlite3_errmsg(db);
        sqlite3_close(db);
        throw std::runtime_error("sqlite3_prepare_v2 failed: " + err);
    }

    QueryResult result;
    const int cols = sqlite3_column_count(stmt);
    for (int c = 0; c < cols; ++c) {
        const unsigned char* name = sqlite3_column_name(stmt, c);
        result.columns.emplace_back(name == nullptr ? "" : reinterpret_cast<const char*>(name));
    }

    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_DONE) {
            break;
        }
        if (rc != SQLITE_ROW) {
            std::string err = sqlite3_errmsg(db);
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            throw std::runtime_error("sqlite3_step failed: " + err);
        }

        std::vector<std::string> row;
        row.reserve(static_cast<std::size_t>(cols));
        for (int c = 0; c < cols; ++c) {
            const unsigned char* value = sqlite3_column_text(stmt, c);
            row.emplace_back(value == nullptr ? "" : reinterpret_cast<const char*>(value));
        }
        result.rows.push_back(std::move(row));
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result;
#else
    (void)sql;
    throw std::runtime_error("SQLite support is disabled. Rebuild with -DSQLBOT_ENABLE_SQLITE3=ON and static sqlite3 linkage.");
#endif
}

std::unique_ptr<DbConnector> CreateConnector(const DatabaseType db_type) {
    switch (db_type) {
        case DatabaseType::SQLite:
            return std::make_unique<SQLiteConnector>();
        case DatabaseType::SqlServer:
            return std::make_unique<SqlServerConnector>();
        case DatabaseType::BigQuery:
            return std::make_unique<BigQueryConnector>();
    }
    throw std::runtime_error("Unsupported database type");
}

std::string ToString(const DatabaseType db_type) {
    switch (db_type) {
        case DatabaseType::SQLite:
            return "sqlite";
        case DatabaseType::SqlServer:
            return "sqlserver";
        case DatabaseType::BigQuery:
            return "bigquery";
    }
    return "unknown";
}

} // namespace sqlbot
