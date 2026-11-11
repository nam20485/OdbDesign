#include "RgbColor.h"
#include <cstdint>

namespace Odb::Lib::FileModel::Design
{
    RgbColor::RgbColor()
        : red(0)
        , green(0)
        , blue(0)
        , noPreference(false)
    {
    }

    RgbColor::RgbColor(const std::string& str)
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

            try
            {
                // valid values are [0,100]
                red = static_cast<uint8_t>(std::stoi(strRed));
                green = static_cast<uint8_t>(std::stoi(strGreen));
                blue = static_cast<uint8_t>(std::stoi(strBlue));
            }
            catch (const std::exception&)
            {
                // non-numeric component; report as unparsable rather than
                // letting stoi escape as an unrelated exception
                return false;
            }

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

    std::string RgbColor::to_string() const
    {
        if (noPreference)
        {
            return "0";
        }
        else
        {
            // each component is a two-digit zero-padded decimal, so the
            // concatenation is exactly the six-character form from_string
            // parses (e.g. (6, 6, 6) -> "060606", not "666")
            auto twoDigits = [](uint8_t value) -> std::string
            {
                if (value < 10) return "0" + std::to_string(value);
                return std::to_string(value);
            };
            // note: a component of exactly 100 emits three digits and cannot
            // round-trip through the six-character form; the ODB matrix
            // samples use two-digit percent values
            return twoDigits(red) + twoDigits(green) + twoDigits(blue);
        }
    }
}