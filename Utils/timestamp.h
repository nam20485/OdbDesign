//
// Created by nam20485 on 6/12/22.
//

#pragma once

#include <string>
#include <chrono>
#include "utils_export.h"

namespace Utils
{
	std::string UTILS_EXPORT make_timestamp(const std::chrono::system_clock::time_point& timepoint);
	std::string UTILS_EXPORT make_timestamp();

	std::chrono::system_clock::time_point UTILS_EXPORT parse_timestamp(const std::string& timestamp, const std::string& format);
}

