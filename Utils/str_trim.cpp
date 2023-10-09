#include "str_trim.h"
#include <iostream>
#include <string>
#include <algorithm>


namespace Utils
{
    // trim from start (in place)
    std::string& str_ltrim(std::string& s)
    {
        auto it = std::find_if(s.begin(), s.end(),
            [](char c) {
                return !std::isspace(c);
            });
        s.erase(s.begin(), it);
        return s;
    }

    // trim from end (in place)
    std::string& str_rtrim(std::string& s)
    {
        auto it = std::find_if(s.rbegin(), s.rend(),
            [](char c) {
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

    // trim from start (copying)
    std::string str_ltrim_copy(std::string s)
    {
        auto copy(s);
        return str_ltrim(copy);
    }

    // trim from end (copying)
    std::string str_rtrim_copy(std::string s)
    {
        auto copy(s);
        return str_rtrim(copy);
    }

    // trim from both ends (copying)
    std::string str_trim_copy(std::string s)
    {
        auto copy(s);
        return str_trim(copy);
    }
}