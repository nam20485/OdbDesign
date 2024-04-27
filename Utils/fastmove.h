#pragma once

#include <filesystem>
#include "utils_export.h"
#include <system_error>

namespace Utils
{
	UTILS_EXPORT std::error_code move(const std::filesystem::path& source, const std::filesystem::path& dest, bool overwriteExisting);
	UTILS_EXPORT std::error_code fastmove(const std::filesystem::path& source, const std::filesystem::path& dest, bool overwriteExisting);
}
