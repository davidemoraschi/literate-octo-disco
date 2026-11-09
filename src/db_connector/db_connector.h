#pragma once

#include <memory>
#include <string>
#include <vector>

namespace sqlbot {

enum class DatabaseType {
    SQLite,
    SqlServer,
    BigQuery
};

struct QueryResult {
    std::vector<std::string> columns;
    std::vector<std::vector<std::string>> rows;
};

/**
 * @brief Abstract database connector for SQL execution.
 */
class DbConnector {
public:
    virtual ~DbConnector() = default;

    /**
     * @brief Connects to the target database.
     * @param connection_string Database-specific connection string.
     */
    virtual void Connect(const std::string& connection_string) = 0;

    /**
     * @brief Executes a SQL query and returns tabular rows.
     * @param sql SQL to execute.
     * @return QueryResult with columns and row values.
     */
    virtual QueryResult Execute(const std::string& sql) = 0;
};

std::unique_ptr<DbConnector> CreateConnector(DatabaseType db_type);
std::string ToString(DatabaseType db_type);

} // namespace sqlbot
