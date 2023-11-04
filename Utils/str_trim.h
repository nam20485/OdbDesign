#pragma once

#include <string>
#include "utils_export.h"


namespace Utils
{
	//// trim from start (in place)
	//UTILS_EXPORT std::string& str_ltrim(std::string& s);
	//// trim from end (in place)
	//UTILS_EXPORT std::string& str_rtrim(std::string& s);
	//// trim from both ends (in place)
	//UTILS_EXPORT std::string& str_trim(std::string& s);

	UTILS_EXPORT std::string& str_ltrim(std::string& s, char toRemove = ' ');
	// trim from end (in place)
	UTILS_EXPORT std::string& str_rtrim(std::string& s, char toRemove = ' ');
	// trim from both ends (in place)
	UTILS_EXPORT std::string& str_trim(std::string& s, char toRemove = ' ');

	//// trim from start (copying)
	//UTILS_EXPORT std::string str_ltrim_copy(std::string s);
	//// trim from end (copying)
	//UTILS_EXPORT std::string str_rtrim_copy(std::string s);
	//// trim from both ends (copying)
	//UTILS_EXPORT std::string str_trim_copy(std::string s);

	UTILS_EXPORT std::string str_ltrim_copy(std::string s, char toRemove = ' ');
	// trim from end (copying)
	UTILS_EXPORT std::string str_rtrim_copy(std::string s, char toRemove = ' ');
	// trim from both ends (copying)
	UTILS_EXPORT std::string str_trim_copy(std::string s, char toRemove = ' ');
}