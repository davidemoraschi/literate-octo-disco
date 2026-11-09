#pragma once

#include "db_connector.h"

#include <string>

namespace sqlbot {

/**
 * @brief SQLite connector using native sqlite3 C API.
 */
class SQLiteConnector final : public DbConnector {
public:
    void Connect(const std::string& connection_string) override;
    QueryResult Execute(const std::string& sql) override;

private:
    std::string db_path_;
};

} // namespace sqlbot
