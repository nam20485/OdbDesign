#pragma once

#include "string_trim.h"
#include <iostream>
#include <string>
#include <algorithm>


namespace Odb::Lib
{
    // trim from start (in place)
    std::string& ltrim(std::string& s)
    {
        auto it = std::find_if(s.begin(), s.end(),
            [](char c) {
                return !std::isspace(c);
            });
        s.erase(s.begin(), it);
        return s;
    }

    // trim from end (in place)
    std::string& rtrim(std::string& s)
    {
        auto it = std::find_if(s.rbegin(), s.rend(),
            [](char c) {
                return !std::isspace(c);
            });
        s.erase(it.base(), s.end());
        return s;
    }

    // trim from both ends (in place)
    std::string& trim(std::string& s)
    {
        return ltrim(rtrim(s));
    }

    // trim from start (copying)
    std::string ltrim_copy(std::string s)
    {
        //auto copy(s);
        return ltrim(std::string(s) );
    }

    // trim from end (copying)
    std::string rtrim_copy(std::string s)
    {
        //auto copy(s);
        return rtrim(std::string(s));
    }

    // trim from both ends (copying)
    std::string trim_copy(std::string s)
    {
        //auto copy(s);
        return trim(std::string(s));
    }
}
