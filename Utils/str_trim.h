#pragma once

#include <string>
#include "export.h"


namespace Utils
{
	// trim from start (in place)
	DECLSPEC std::string& str_ltrim(std::string& s);
	// trim from end (in place)
	DECLSPEC std::string& str_rtrim(std::string& s);
	// trim from both ends (in place)
	DECLSPEC std::string& str_trim(std::string& s);

	// trim from start (copying)
	DECLSPEC std::string str_ltrim_copy(std::string s);
	// trim from end (copying)
	DECLSPEC std::string str_rtrim_copy(std::string s);
	// trim from both ends (copying)
	DECLSPEC std::string str_trim_copy(std::string s);
}