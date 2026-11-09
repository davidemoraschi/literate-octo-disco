#pragma once
#include "utils/config.h"
#include <string>
namespace sqlbot {
	/**
	 * @brief Builds a human-readable, secret-free summary of the active connection.
	 *
	 * Lines are separated by '\n'. Passwords, tokens and other secrets are never
	 * included in the output.
	 */
	std::string DescribeConnection(const RuntimeConfig& config);
} // namespace sqlbot