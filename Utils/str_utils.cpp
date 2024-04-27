#include "str_utils.h"
#include "str_utils.h"
#include "str_utils.h"
#include "str_utils.h"
#include "str_utils.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <vector>


namespace Utils
{

    static bool case_insensitive_compare(const std::string& s1, const std::string& s2);

    //! std::isspace(char) can only handle chars in the range [0,255] 
    //! so we need to cast the it's argument to unsigned char
    // trim from start (in place)
    std::string& str_ltrim(std::string& s)
    {
        auto it = std::find_if(s.begin(), s.end(),
            [](unsigned char c) {
                return !std::isspace(c);
            });
        s.erase(s.begin(), it);
        return s;
    }

    // trim from end (in place)
    std::string& str_rtrim(std::string& s)
    {
        auto it = std::find_if(s.rbegin(), s.rend(),
            [](unsigned char c) {
                return !std::isspace(c);
            });
        s.erase(it.base(), s.end());
        return s;
    }

    // trim from both ends (in place)
    std::string& str_trim(std::string& s)
    {
        return str_ltrim(str_rtrim(s));
    }

    std::string& str_ltrim(std::string& s, /*unsigned*/ char toRemove)
    {
        auto it = std::find_if(s.begin(), s.end(),
            [toRemove](/*unsigned*/ char c) {
                return c != toRemove;
            });
        s.erase(s.begin(), it);
        return s;
    }

    // trim from end (in place)
    std::string& str_rtrim(std::string& s, /*unsigned*/ char toRemove)
    {
        auto it = std::find_if(s.rbegin(), s.rend(),
            [toRemove](/*unsigned*/ char c) {
                return c != toRemove;
            });
        s.erase(it.base(), s.end());
        return s;
    }

    // trim from both ends (in place)
    std::string& str_trim(std::string& s, /*unsigned*/ char toRemove)
    {
        return str_ltrim(str_rtrim(s, toRemove), toRemove);
    }

    // trim from start (copying)
    std::string str_ltrim_copy(const std::string& s)
    {
        auto copy(s);
        return str_ltrim(copy);
    }

    // trim from end (copying)
    std::string str_rtrim_copy(const std::string& s)
    {
        auto copy(s);
        return str_rtrim(copy);
    }

    // trim from both ends (copying)
    std::string str_trim_copy(const std::string& s)
    {
        auto copy(s);
        return str_trim(copy);
    }

    std::string str_ltrim_copy(const std::string& s, /*unsigned*/ char toRemove)
    {
        auto copy(s);
        return str_ltrim(copy, toRemove);
    }

    // trim from end (copying)
    std::string str_rtrim_copy(const std::string& s, /*unsigned*/ char toRemove)
    {
        auto copy(s);
        return str_rtrim(copy, toRemove);
    }

    // trim from both ends (copying)
    std::string str_trim_copy(const std::string& s, /*unsigned*/ char toRemove)
    {
        auto copy(s);
        return str_trim(copy, toRemove);
    }

    void str_to_lower(std::string& s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    }

    void str_to_upper(std::string& s)
    {
		std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); });
	}   

    std::string str_to_lower_copy(const std::string& s)
    {
        std::string copy = s;
        std::transform(copy.begin(), copy.end(), copy.begin(), [](unsigned char c) { return std::tolower(c); });
        return copy;
    }

    std::string str_to_upper_copy(const std::string& s)
    {
        std::string copy;
        std::transform(copy.begin(), copy.end(), copy.begin(), [](unsigned char c) { return std::toupper(c); });
        return copy;
    }   

    bool str_iequals(const std::string& s1, const std::string& s2)
    {
        return case_insensitive_compare(s1, s2);
    }

    std::vector<std::string>::iterator find_str_icmp(std::vector<std::string>::iterator first, std::vector<std::string>::iterator last, const std::string& val)
    {
        return std::find_if(first, last, [&val](const std::string& s) { return case_insensitive_compare(s, val); });
    }

    std::vector<std::string>::const_iterator find_str_icmp(const std::vector<std::string>::const_iterator first, const std::vector<std::string>::const_iterator last, const std::string& val)
    {
        return std::find_if(first, last, [&val](const std::string& s) { return case_insensitive_compare(s, val); });
    }

    static bool case_insensitive_compare(const std::string& s1, const std::string& s2)
    {
        return std::equal(
            s1.begin(), s1.end(),
            s2.begin(), s2.end(),
            [](char c1, char c2)
            {
                return std::tolower(c1) == std::tolower(c2);
            });
    }

  //  UTILS_EXPORT std::string Utils::str_replace(const std::string& s, const std::string& from, const std::string& to)
  //  {
  //      std::string result;
		//result.reserve(s.length()); // avoids a few memory allocations

		//std::string::size_type lastPos = 0;
		//std::string::size_type findPos;

  //      while (std::string::npos != (findPos = s.find(from, lastPos)))
  //      {
		//	result.append(s, lastPos, findPos - lastPos);
		//	result += to;
		//	lastPos = findPos + from.length();
		//}

		//// Care for the rest after last occurrence
		//result += s.substr(lastPos);

		//return result;
  //  }
}
