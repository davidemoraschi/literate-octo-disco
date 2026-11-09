#pragma once

#include <chrono>
#include <string>

namespace sqlbot {

/**
 * @brief MCP protocol version selected at runtime.
 */
enum class McpVersion {
    V1_0,
    V1_1
};

/**
 * @brief MCP client that negotiates protocol and translates NL to SQL.
 */
class McpClient {
public:
    McpClient(std::string server_url, bool allow_insecure, std::chrono::seconds timeout);

    /**
     * @brief Performs MCP handshake and version negotiation.
     */
    void Initialize();

    /**
     * @brief Sends natural-language request and retrieves SQL text.
     */
    std::string GenerateSql(const std::string& natural_language,
                            const std::string& database_type,
                            const std::string& connection_hint) const;

    [[nodiscard]] McpVersion Version() const;

private:
    std::string server_url_;
    bool allow_insecure_;
    std::chrono::seconds timeout_;
    McpVersion version_;
};

} // namespace sqlbot
