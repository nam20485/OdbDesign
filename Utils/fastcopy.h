#pragma once

#include <filesystem>
#include "utils_export.h"
#include <system_error>

namespace Utils
{
	UTILS_EXPORT std::error_code copy(const std::filesystem::path& source, const std::filesystem::path& dest, bool overwriteExisting);
	UTILS_EXPORT std::error_code fastcopy(const std::filesystem::path& source, const std::filesystem::path& dest, bool overwriteExisting);
}
