#pragma once

#include "db_connector/db_connector.h"

#include <chrono>
#include <optional>
#include <string>

namespace sqlbot {

struct RuntimeConfig {
    std::string mcp_server = "mcp://127.0.0.1:8080";
    DatabaseType database_type = DatabaseType::SQLite;
    std::string connection_string;
    bool insecure = false;
    std::chrono::seconds timeout{30};
    bool gui_mode = false;
    std::optional<std::string> nl_prompt;
};

/**
 * @brief Loads CLI/config.json options for sqlbot runtime.
 */
RuntimeConfig LoadRuntimeConfig(int argc, const char* const* argv);

} // namespace sqlbot
