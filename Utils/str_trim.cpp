#include "str_trim.h"
#include <iostream>
#include <string>
#include <algorithm>


namespace Utils
{
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

    std::string str_ltrim_copy(std::string s, /*unsigned*/ char toRemove)
    {
        auto copy(s);
        return str_ltrim(copy, toRemove);
    }

    // trim from end (copying)
    std::string str_rtrim_copy(std::string s, /*unsigned*/ char toRemove)
    {
        auto copy(s);
        return str_rtrim(copy, toRemove);
    }

    // trim from both ends (copying)
    std::string str_trim_copy(std::string s, /*unsigned*/ char toRemove)
    {
        auto copy(s);
        return str_trim(copy, toRemove);
    }
}
