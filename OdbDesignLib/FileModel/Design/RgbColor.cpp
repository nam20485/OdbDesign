#include "RgbColor.h"
#include <cstdint>
#include <charconv>
#include <stdexcept>

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
            // parse each two-digit component with from_chars: full
            // consumption (rejects "-1", "+1", "1x"), no exceptions, and
            // an explicit [0,100] domain check on the result
            int components[3] = { 0, 0, 0 };
            for (int i = 0; i < 3; ++i)
            {
                const auto* first = str.data() + i * 2;
                const auto* last = first + 2;
                const auto [ptr, ec] = std::from_chars(first, last, components[i]);
                if (ec != std::errc() || ptr != last || components[i] < 0 || components[i] > 100)
                {
                    return false;
                }
            }

            red = static_cast<uint8_t>(components[0]);
            green = static_cast<uint8_t>(components[1]);
            blue = static_cast<uint8_t>(components[2]);
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
            // the six-character form holds exactly two digits per component;
            // a value of 100 is unrepresentable and must fail loudly rather
            // than emit a three-digit run our own parser rejects
            if (red > 99 || green > 99 || blue > 99)
            {
                throw std::domain_error("color component of 100 is not representable in the two-digit ODB COLOR form");
            }
            auto twoDigits = [](uint8_t value) -> std::string
            {
                if (value < 10) return "0" + std::to_string(value);
                return std::to_string(value);
            };
            return twoDigits(red) + twoDigits(green) + twoDigits(blue);
        }
    }
}