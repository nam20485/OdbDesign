#pragma once

#include <filesystem>
#include "utils_export.h"
#include <system_error>

namespace Utils
{
	UTILS_EXPORT void move_file(const std::filesystem::path& source, const std::filesystem::path& dest, bool overwriteExisting, std::error_code& ec);
	UTILS_EXPORT void fastmove_file(const std::filesystem::path& source, const std::filesystem::path& dest, bool overwriteExisting, std::error_code& ec);
}
