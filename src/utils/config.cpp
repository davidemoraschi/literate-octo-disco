#include "utils/config.h"

#include "utils/string_utils.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace sqlbot {
namespace {

DatabaseType ParseDbType(const std::string& value) {
    if (value == "sqlite") {
        return DatabaseType::SQLite;
    }
    if (value == "sqlserver") {
        return DatabaseType::SqlServer;
    }
    if (value == "bigquery") {
        return DatabaseType::BigQuery;
    }
    throw std::runtime_error("Unsupported database type: " + value);
}

void ParseDbSpec(const std::string& spec, RuntimeConfig& cfg) {
    const auto sep = spec.find(':');
    if (sep == std::string::npos) {
        throw std::runtime_error("--db expects '<type>:<connection_string>'");
    }
    cfg.database_type = ParseDbType(spec.substr(0, sep));
    cfg.connection_string = spec.substr(sep + 1);
}

std::string ReadFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        return {};
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

void LoadConfigJson(const std::string& path, RuntimeConfig& cfg) {
    const std::string content = ReadFile(path);
    if (content.empty()) {
        return;
    }

    try {
        cfg.mcp_server = ExtractJsonString(content, "mcp_server");
    } catch (...) {
    }

    try {
        cfg.database_type = ParseDbType(ExtractJsonString(content, "database_type"));
    } catch (...) {
    }

    try {
        cfg.connection_string = ExtractJsonString(content, "connection_string");
    } catch (...) {
    }
}

} // namespace

RuntimeConfig LoadRuntimeConfig(const int argc, const char* const* argv) {
    RuntimeConfig cfg;
    std::string config_path = "config.json";

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg.rfind("--config=", 0) == 0) {
            config_path = arg.substr(9);
        }
    }

    LoadConfigJson(config_path, cfg);

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--gui") {
            cfg.gui_mode = true;
        } else if (arg == "--insecure") {
            cfg.insecure = true;
        } else if (arg.rfind("--mcp=", 0) == 0) {
            cfg.mcp_server = arg.substr(6);
        } else if (arg.rfind("--db=", 0) == 0) {
            ParseDbSpec(arg.substr(5), cfg);
        } else if (arg.rfind("--timeout=", 0) == 0) {
            cfg.timeout = std::chrono::seconds(std::stoi(arg.substr(10)));
        } else if (arg.rfind("--config=", 0) == 0) {
            continue;
        } else if (arg == "--help" || arg == "-h") {
            continue;
        } else if (arg.rfind("--", 0) == 0) {
            throw std::runtime_error("Unknown option: " + arg);
        } else {
            cfg.nl_prompt = arg;
        }
    }

    return cfg;
}

} // namespace sqlbot
