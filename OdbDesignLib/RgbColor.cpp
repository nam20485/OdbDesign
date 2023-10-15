#include "RgbColor.h"

namespace Odb::Lib::FileModel::Design
{
    RgbColor::RgbColor()
        : noPreference(false)
        , red(0)
        , green(0)
        , blue(0)
    {
    }

    inline RgbColor::RgbColor(const std::string& str)
        : RgbColor()
    {
        from_string(str);
    }

    bool RgbColor::from_string(const std::string& str)
    {
        if (str.length() == 6)
        {
            auto strRed = str.substr(0, 2);
            auto strGreen = str.substr(2, 2);
            auto strBlue = str.substr(4, 2);

            // valid values are [0,100]
            red = static_cast<uint8_t>(std::stoi(strRed));
            blue = static_cast<uint8_t>(std::stoi(strBlue));
            green = static_cast<uint8_t>(std::stoi(strGreen));

            noPreference = false;
            return true;
        }
        else if (str.length() == 1)
        {
            if (str[0] == '0')
            {
                noPreference = true;
                return true;
            }
        }

        return false;
    }

    inline std::string RgbColor::to_string()
    {
        if (noPreference)
        {
            return "0";
        }
        else
        {
            return std::to_string(red) + std::to_string(green) + std::to_string(blue);
        }
    }
}