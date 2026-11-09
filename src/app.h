#pragma once

#include "utils/config.h"

#include <string>

namespace sqlbot {

/**
 * @brief Executes NL->SQL->DB flow once and returns formatted output.
 */
std::string RunSingleRequest(const RuntimeConfig& config, const std::string& natural_language);

/**
 * @brief Prints CLI help text.
 */
void PrintUsage();

} // namespace sqlbot
