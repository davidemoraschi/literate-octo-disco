#pragma once

#include <string>

namespace sqlbot {

std::string Trim(std::string value);
std::string EscapeJson(const std::string& input);
std::string ExtractJsonString(const std::string& json, const std::string& key);

} // namespace sqlbot
