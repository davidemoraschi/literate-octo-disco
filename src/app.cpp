#include "app.h"

#include "db_connector/db_connector.h"
#include "mcp_client.h"

#include <algorithm>
#include <future>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace sqlbot {
namespace {

std::string QuoteSql(const std::string& sql) {
    if (sql.empty()) {
        throw std::runtime_error("MCP returned an empty SQL statement.");
    }
    if (sql.find(';') != std::string::npos && sql.find(";") != sql.size() - 1) {
        throw std::runtime_error("Refusing multi-statement SQL for safety.");
    }
    return sql;
}

std::string FormatResultTable(const QueryResult& result) {
    if (result.columns.empty()) {
        return "Query executed successfully (no tabular result).";
    }

    std::vector<std::size_t> widths;
    widths.reserve(result.columns.size());
    for (const auto& col : result.columns) {
        widths.push_back(col.size());
    }

    for (const auto& row : result.rows) {
        for (std::size_t i = 0; i < row.size() && i < widths.size(); ++i) {
            widths[i] = std::max(widths[i], row[i].size());
        }
    }

    std::ostringstream out;
    for (std::size_t i = 0; i < result.columns.size(); ++i) {
        out << std::left << std::setw(static_cast<int>(widths[i] + 2)) << result.columns[i];
    }
    out << "\n";

    for (const auto width : widths) {
        out << std::string(width, '-') << "  ";
    }
    out << "\n";

    for (const auto& row : result.rows) {
        for (std::size_t i = 0; i < result.columns.size(); ++i) {
            const std::string value = i < row.size() ? row[i] : "";
            out << std::left << std::setw(static_cast<int>(widths[i] + 2)) << value;
        }
        out << "\n";
    }
    return out.str();
}

} // namespace

std::string RunSingleRequest(const RuntimeConfig& config, const std::string& natural_language) {
    McpClient mcp(config.mcp_server, config.insecure, config.timeout);

    auto sql_future = std::async(std::launch::async, [&]() {
        mcp.Initialize();
        return mcp.GenerateSql(natural_language, ToString(config.database_type), config.connection_string);
    });

    const std::string sql = QuoteSql(sql_future.get());
    auto connector = CreateConnector(config.database_type);

    auto result_future = std::async(std::launch::async, [&]() {
        connector->Connect(config.connection_string);
        return connector->Execute(sql);
    });

    const QueryResult result = result_future.get();

    std::ostringstream output;
    output << "MCP SQL: " << sql << "\n";
    output << FormatResultTable(result);
    return output.str();
}

void PrintUsage() {
    std::cout
        << "sqlbot --db=<sqlite|sqlserver|bigquery>:<connection> [--mcp=<url>] [--timeout=30] [--insecure] \"question\"\n"
        << "sqlbot --gui [--db=...] [--mcp=...]\n";
}

} // namespace sqlbot
