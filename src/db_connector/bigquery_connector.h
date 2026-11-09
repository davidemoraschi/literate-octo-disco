#pragma once

#include "db_connector.h"

#include <string>

namespace sqlbot {

/**
 * @brief BigQuery connector placeholder for gRPC/OAuth2 integration.
 */
class BigQueryConnector final : public DbConnector {
public:
    void Connect(const std::string& connection_string) override;
    QueryResult Execute(const std::string& sql) override;

private:
    std::string connection_string_;
};

} // namespace sqlbot
