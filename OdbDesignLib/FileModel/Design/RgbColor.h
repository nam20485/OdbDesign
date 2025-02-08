#pragma once

#include <string>
#include "../../odbdesign_export.h"
#include <cstdint>

namespace Odb::Lib::FileModel::Design
{
    /// @class struct for representing preferred layer color
    /// @brief each color value ranges from 0% - 100%            
    /// noPreference is true if the Odb++ file does not specify a color
    struct ODBDESIGN_EXPORT RgbColor
    {        
        uint8_t red;
        uint8_t green;
        uint8_t blue;

        bool noPreference;

        RgbColor();
        explicit RgbColor(const std::string& str);

        bool from_string(const std::string& str);
        std::string to_string() const;
    };
}
