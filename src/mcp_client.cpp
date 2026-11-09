#include "mcp_client.h"

#include "utils/string_utils.h"

#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace sqlbot {
namespace {

struct Endpoint {
    std::string scheme;
    std::string host;
    std::string port;
};

Endpoint ParseEndpoint(const std::string& url) {
    const auto scheme_sep = url.find("://");
    if (scheme_sep == std::string::npos) {
        throw std::runtime_error("Invalid MCP server URL: " + url);
    }

    Endpoint ep;
    ep.scheme = url.substr(0, scheme_sep);
    std::string rest = url.substr(scheme_sep + 3);
    const auto slash = rest.find('/');
    if (slash != std::string::npos) {
        rest = rest.substr(0, slash);
    }

    const auto colon = rest.rfind(':');
    if (colon == std::string::npos) {
        ep.host = rest;
        ep.port = (ep.scheme == "https" || ep.scheme == "mcp+tls") ? "443" : "80";
    } else {
        ep.host = rest.substr(0, colon);
        ep.port = rest.substr(colon + 1);
    }

    if (ep.host.empty() || ep.port.empty()) {
        throw std::runtime_error("Invalid MCP endpoint host/port");
    }
    return ep;
}

class SocketSession {
public:
    explicit SocketSession(const std::chrono::seconds timeout) {
#ifdef _WIN32
        WSADATA data{};
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
            throw std::runtime_error("WSAStartup failed");
        }
#endif
        timeout_ms_ = static_cast<int>(timeout.count() * 1000);
    }

    ~SocketSession() {
        Close();
#ifdef _WIN32
        WSACleanup();
#endif
    }

    void Connect(const std::string& host, const std::string& port) {
        addrinfo hints{};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        addrinfo* result = nullptr;
        if (getaddrinfo(host.c_str(), port.c_str(), &hints, &result) != 0) {
            throw std::runtime_error("getaddrinfo failed for MCP endpoint");
        }

        for (addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
            socket_ = static_cast<int>(socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol));
            if (socket_ < 0) {
                continue;
            }

            SetTimeouts();
            if (connect(socket_, ptr->ai_addr, static_cast<int>(ptr->ai_addrlen)) == 0) {
                freeaddrinfo(result);
                return;
            }
            Close();
        }

        freeaddrinfo(result);
        throw std::runtime_error("Unable to connect to MCP server");
    }

    void SendLine(const std::string& line) const {
        const std::string payload = line + "\n";
#ifdef _WIN32
        const int sent = send(socket_, payload.c_str(), static_cast<int>(payload.size()), 0);
#else
        const ssize_t sent = send(socket_, payload.c_str(), payload.size(), 0);
#endif
        if (sent < 0) {
            throw std::runtime_error("Failed to send MCP payload");
        }
    }

    std::string ReadLine() const {
        std::string response;
        char ch = '\0';
        while (true) {
#ifdef _WIN32
            const int rc = recv(socket_, &ch, 1, 0);
#else
            const ssize_t rc = recv(socket_, &ch, 1, 0);
#endif
            if (rc <= 0) {
                break;
            }
            if (ch == '\n') {
                break;
            }
            response.push_back(ch);
        }
        return Trim(response);
    }

private:
    void SetTimeouts() const {
#ifdef _WIN32
        setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout_ms_), sizeof(timeout_ms_));
        setsockopt(socket_, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout_ms_), sizeof(timeout_ms_));
#else
        timeval tv{};
        tv.tv_sec = timeout_ms_ / 1000;
        tv.tv_usec = (timeout_ms_ % 1000) * 1000;
        setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(socket_, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
    }

    void Close() {
        if (socket_ < 0) {
            return;
        }
#ifdef _WIN32
        closesocket(socket_);
#else
        close(socket_);
#endif
        socket_ = -1;
    }

    int socket_{-1};
    int timeout_ms_{30000};
};

std::string ExtractSqlFromResponse(const std::string& response) {
    if (response.empty()) {
        throw std::runtime_error("MCP response is empty");
    }
    if (response.front() == '{') {
        return ExtractJsonString(response, "sql");
    }
    return response;
}

} // namespace

McpClient::McpClient(std::string server_url, const bool allow_insecure, const std::chrono::seconds timeout)
    : server_url_(std::move(server_url)), allow_insecure_(allow_insecure), timeout_(timeout), version_(McpVersion::V1_1) {
}

void McpClient::Initialize() {
    const Endpoint endpoint = ParseEndpoint(server_url_);
    if ((endpoint.scheme == "https" || endpoint.scheme == "mcp+tls") && !allow_insecure_) {
        throw std::runtime_error(
            "TLS MCP transport requested but no built-in TLS stack is linked in this build. "
            "Use --insecure for plaintext testing or rebuild with TLS support.");
    }

    SocketSession session(timeout_);
    session.Connect(endpoint.host, endpoint.port);
    session.SendLine("MCP/1.1 IDENTIFY");

    const std::string ready = session.ReadLine();
    if (ready.find("MCP/1.1 READY") != std::string::npos) {
        version_ = McpVersion::V1_1;
        return;
    }
    if (ready.find("MCP/1.0 READY") != std::string::npos) {
        version_ = McpVersion::V1_0;
        return;
    }

    throw std::runtime_error("MCP handshake failed: " + ready);
}

std::string McpClient::GenerateSql(const std::string& natural_language,
                                   const std::string& database_type,
                                   const std::string& connection_hint) const {
    const Endpoint endpoint = ParseEndpoint(server_url_);
    SocketSession session(timeout_);
    session.Connect(endpoint.host, endpoint.port);

    const std::string request =
        (version_ == McpVersion::V1_1 ? "MCP/1.1 QUERY " : "MCP/1.0 QUERY ") +
        std::string("{\"database\":\"") + EscapeJson(database_type) +
        "\",\"connection\":\"" + EscapeJson(connection_hint) +
        "\",\"prompt\":\"" + EscapeJson(natural_language) + "\"}";

    session.SendLine(request);
    const std::string response = session.ReadLine();
    return ExtractSqlFromResponse(response);
}

McpVersion McpClient::Version() const {
    return version_;
}

} // namespace sqlbot
