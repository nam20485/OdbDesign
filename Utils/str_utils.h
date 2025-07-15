#pragma once

#include <string>
#include "utils_export.h"
#include <vector>


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
	UTILS_EXPORT std::string str_ltrim_copy(const std::string& s);
	// trim from end (copying)
	UTILS_EXPORT std::string str_rtrim_copy(const std::string& s);
	// trim from both ends (copying)
	UTILS_EXPORT std::string str_trim_copy(const std::string& s);

	UTILS_EXPORT std::string str_ltrim_copy(const std::string&, /*unsigned*/ char toRemove);
	// trim from end (copying)
	UTILS_EXPORT std::string str_rtrim_copy(const std::string&, /*unsigned*/ char toRemove);
	// trim from both ends (copying)
	UTILS_EXPORT std::string str_trim_copy(const std::string&, /*unsigned*/ char toRemove);

	UTILS_EXPORT void str_to_lower(std::string& s);
	UTILS_EXPORT void str_to_upper(std::string& s);

	UTILS_EXPORT std::string str_to_lower_copy(const std::string& s);
	UTILS_EXPORT std::string str_to_upper_copy(const std::string& s);

	//UTILS_EXPORT std::string str_replace(const std::string& s, const std::string& from, const std::string& to);	

	UTILS_EXPORT bool str_iequals(const std::string& s1, const std::string& s2);	

	UTILS_EXPORT std::vector<std::string>::iterator find_str_icmp(std::vector<std::string>::iterator first, 
																  std::vector<std::string>::iterator last, 
																  const std::string& val);

	UTILS_EXPORT std::vector<std::string>::const_iterator find_str_icmp(const std::vector<std::string>::const_iterator first,
																		const std::vector<std::string>::const_iterator last,
																		const std::string& val);

}