//
// Created by nam20485 on 6/12/22.
//

#pragma once

#include <string>
#include <chrono>
#include "utils_export.h"

using namespace std::chrono;

namespace Utils
{
	std::string UTILS_EXPORT make_timestamp(const system_clock::time_point& timepoint);
	std::string UTILS_EXPORT make_timestamp();

	system_clock::time_point UTILS_EXPORT parse_timestamp(const std::string& timestamp, const std::string& format);
}

