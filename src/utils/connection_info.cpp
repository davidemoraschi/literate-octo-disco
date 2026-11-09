#include "utils/connection_info.h"
#include "utils/string_utils.h"
#include <cctype>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>
namespace sqlbot {
	namespace {
		using KeyValues = std::vector<std::pair<std::string, std::string>>;
		std::string Lower(std::string value) {
			for (auto& ch : value) {
				ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
			}
			return value;
		}
		bool IsSensitiveKey(const std::string& key) {
			const std::string k = Lower(key);
			for (const char* marker : { "pwd", "password", "secret", "token", "key" }) {
				if (k.find(marker) != std::string::npos) {
					return true;
				}
			}
			return false;
		}
		// Parses ODBC-style "Key=value;Key={va;lue};" pairs.
		KeyValues ParseKeyValues(const std::string& text) {
			KeyValues out;
			std::size_t pos = 0;
			while (pos < text.size()) {
				const std::size_t eq = text.find('=', pos);
				if (eq == std::string::npos) {
					break;
				}
				std::string key = Trim(text.substr(pos, eq - pos));
				pos = eq + 1;
				std::string value;
				if (pos < text.size() && text[pos] == '{') {
					const std::size_t close = text.find('}', pos + 1);
					const std::size_t end = close == std::string::npos ? text.size() : close;
					value = text.substr(pos + 1, end - pos - 1);
					const std::size_t semi = text.find(';', end);
					pos = semi == std::string::npos ? text.size() : semi + 1;
				}
				else {
					const std::size_t semi = text.find(';', pos);
					const std::size_t end = semi == std::string::npos ? text.size() : semi;
					value = Trim(text.substr(pos, end - pos));
					pos = end + 1;
				}
				if (!key.empty()) {
					out.emplace_back(std::move(key), std::move(value));
				}
			}
			return out;
		}
		std::string Lookup(const KeyValues& kv, std::initializer_list<const char*> names) {
			for (const auto& [key, value] : kv) {
				const std::string k = Lower(key);
				for (const char* name : names) {
					if (k == Lower(name)) {
						return value;
					}
				}
			}
			return {};
		}
		void AddLine(std::vector<std::string>& lines, const std::string& label, const std::string& value) {
			lines.push_back(label + ": " + (value.empty() ? std::string("(not set)") : value));
		}
		void DescribeSqlServer(const RuntimeConfig& config, std::vector<std::string>& lines) {
			const KeyValues kv = ParseKeyValues(config.connection_string);
			lines.emplace_back("Database: SQL Server (ODBC)");
			AddLine(lines, "Server", Lookup(kv, { "Server", "Data Source", "Address", "Addr", "Network Address" }));
			AddLine(lines, "Database", Lookup(kv, { "Database", "Initial Catalog" }));
			std::string user = Lookup(kv, { "UID", "User Id", "User" });
			if (user.empty() && Lower(Lookup(kv, { "Trusted_Connection", "Integrated Security" })) == "yes") {
				user = "Windows authentication";
			}
			AddLine(lines, "User", user);
			AddLine(lines, "Driver", Lookup(kv, { "Driver" }));
		}
		void DescribeSqlite(const RuntimeConfig& config, std::vector<std::string>& lines) {
			lines.emplace_back("Database: SQLite");
			AddLine(lines, "File", config.connection_string);
		}
		void DescribeBigQuery(const RuntimeConfig& config, std::vector<std::string>& lines) {
			lines.emplace_back("Database: BigQuery");
			const KeyValues kv = ParseKeyValues(config.connection_string);
			if (kv.empty()) {
				AddLine(lines, "Connection", config.connection_string);
				return;
			}
			for (const auto& [key, value] : kv) {
				AddLine(lines, key, IsSensitiveKey(key) ? std::string("****") : value);
			}
		}
	} // namespace
	std::string DescribeConnection(const RuntimeConfig& config) {
		std::vector<std::string> lines;
		switch (config.database_type) {
		case DatabaseType::SqlServer:
			DescribeSqlServer(config, lines);
			break;
		case DatabaseType::SQLite:
			DescribeSqlite(config, lines);
			break;
		case DatabaseType::BigQuery:
			DescribeBigQuery(config, lines);
			break;
		}
		lines.push_back("MCP: " + config.mcp_server + (config.insecure ? " (TLS checks disabled)" : ""));
		std::string out;
		for (const auto& line : lines) {
			out += line;
			out += '\n';
		}
		if (!out.empty()) {
			out.pop_back();
		}
		return out;
	}
} // namespace sqlbot