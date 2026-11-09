#include "utils/string_utils.h"

#include <cctype>
#include <stdexcept>

namespace sqlbot {

std::string Trim(std::string value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
        value.erase(value.begin());
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
        value.pop_back();
    }
    return value;
}

std::string EscapeJson(const std::string& input) {
    std::string out;
    out.reserve(input.size() + 16);
    for (const char c : input) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out.push_back(c); break;
        }
    }
    return out;
}

std::string ExtractJsonString(const std::string& json, const std::string& key) {
    const std::string quoted_key = "\"" + key + "\"";
    const std::size_t key_pos = json.find(quoted_key);
    if (key_pos == std::string::npos) {
        throw std::runtime_error("Missing JSON key: " + key);
    }

    const std::size_t colon = json.find(':', key_pos + quoted_key.size());
    if (colon == std::string::npos) {
        throw std::runtime_error("Invalid JSON payload: missing ':'");
    }

    std::size_t start_quote = json.find('"', colon + 1);
    if (start_quote == std::string::npos) {
        throw std::runtime_error("Invalid JSON payload: missing string value");
    }
    ++start_quote;

    std::string value;
    bool escaping = false;
    for (std::size_t i = start_quote; i < json.size(); ++i) {
        const char c = json[i];
        if (escaping) {
            switch (c) {
                case 'n': value.push_back('\n'); break;
                case 'r': value.push_back('\r'); break;
                case 't': value.push_back('\t'); break;
                case '"': value.push_back('"'); break;
                case '\\': value.push_back('\\'); break;
                default: value.push_back(c); break;
            }
            escaping = false;
            continue;
        }

        if (c == '\\') {
            escaping = true;
            continue;
        }
        if (c == '"') {
            return value;
        }
        value.push_back(c);
    }

    throw std::runtime_error("Invalid JSON payload: unterminated string");
}

} // namespace sqlbot
