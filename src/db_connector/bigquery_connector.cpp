#include "db_connector/bigquery_connector.h"

#include <stdexcept>

namespace sqlbot {

void BigQueryConnector::Connect(const std::string& connection_string) {
    if (connection_string.empty()) {
        throw std::runtime_error("BigQuery connection string is empty.");
    }
    connection_string_ = connection_string;
}

QueryResult BigQueryConnector::Execute(const std::string& sql) {
    if (connection_string_.empty()) {
        throw std::runtime_error("BigQuery is not connected.");
    }
    (void)sql;
    throw std::runtime_error(
        "BigQuery connector requires gRPC/OAuth2 static integration. "
        "Provide a build with native BigQuery transport support.");
}

} // namespace sqlbot
