#pragma once

#include <filesystem>
#include <string>
#include "utils_export.h"

namespace Utils
{
	UTILS_EXPORT std::error_code copy(const std::filesystem::path& source, const std::filesystem::path& dest, bool overwriteExisting);
	//UTILS_EXPORT std::error_code copy(const std::string& source, const std::string& dest, bool overwriteExisting);

	//UTILS_EXPORT std::error_code fastcopy(const std::string& source, const std::string& dest, bool overwriteExisting);
	UTILS_EXPORT std::error_code fastcopy(const std::filesystem::path& source, const std::filesystem::path& dest, bool overwriteExisting);
}
