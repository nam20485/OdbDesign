#pragma once

#include <string>
#include "utils_export.h"


namespace Utils
{
	// trim from start (in place)
	UTILS_EXPORT std::string& str_ltrim(std::string& s);
	// trim from end (in place)
	UTILS_EXPORT std::string& str_rtrim(std::string& s);
	// trim from both ends (in place)
	UTILS_EXPORT std::string& str_trim(std::string& s);

	UTILS_EXPORT std::string& str_ltrim(std::string& s, /*unsigned*/ char toRemove);
	// trim from end (in place)
	UTILS_EXPORT std::string& str_rtrim(std::string& s, /*unsigned*/ char toRemove);
	// trim from both ends (in place)
	UTILS_EXPORT std::string& str_trim(std::string& s, /*unsigned*/ char toRemove);

	// trim from start (copying)
	UTILS_EXPORT std::string str_ltrim_copy(std::string s);
	// trim from end (copying)
	UTILS_EXPORT std::string str_rtrim_copy(std::string s);
	// trim from both ends (copying)
	UTILS_EXPORT std::string str_trim_copy(std::string s);

	UTILS_EXPORT std::string str_ltrim_copy(std::string s, /*unsigned*/ char toRemove);
	// trim from end (copying)
	UTILS_EXPORT std::string str_rtrim_copy(std::string s, /*unsigned*/ char toRemove);
	// trim from both ends (copying)
	UTILS_EXPORT std::string str_trim_copy(std::string s, /*unsigned*/ char toRemove);

	UTILS_EXPORT void str_to_lower(std::string& s);
	UTILS_EXPORT void str_to_upper(std::string& s);

	UTILS_EXPORT std::string str_to_lower_copy(const std::string& s);
	UTILS_EXPORT std::string str_to_upper_copy(const std::string& s);
}