#pragma once

#include "db_connector.h"

#include <string>

namespace sqlbot {

/**
 * @brief SQL Server connector using Windows ODBC.
 */
class SqlServerConnector final : public DbConnector {
public:
    void Connect(const std::string& connection_string) override;
    QueryResult Execute(const std::string& sql) override;

private:
    std::string connection_string_;
};

} // namespace sqlbot
