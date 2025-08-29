//
// Created by Jandody on 08/26/2025.
//
#pragma once

#include <string>
#include <map>
#include <filesystem>
#include <memory>
#include <EnumMap.h>

#include "../../odbdesign_export.h"
#include "../../IProtoBuffable.h"
#include "../../ProtoBuf/toolsfile.pb.h"
#include "../IStreamSaveable.h"

namespace Odb::Lib::FileModel::Design
{
    class ODBDESIGN_EXPORT ToolsFile : public IProtoBuffable<Odb::Lib::Protobuf::ToolsFile>, public IStreamSaveable
    {
    public:
        ToolsFile();
        ~ToolsFile();

        // ToolsRecord
        struct ToolsRecord : public IProtoBuffable<Odb::Lib::Protobuf::ToolsFile::ToolsRecord>
        {
            enum class Type
            {
                Plated,
                NoPlated,
                Via,
            };

            enum class Type2
            {
                Standard,
                PressFit,
                Photo,
                Laser,
            };

            // data members
            unsigned int toolNum;	// reference number of TOOLS in tools file
            Type type;
            Type2 type2;

            // Allowed tolerances
            double minTOL;
            double maxTOL;

            std::string driilBit;
            double FinishSize;
            double DrillSize;

            inline static const char* RECORD_TOKEN = "TOOLS";

            inline static const char* NUM_KEY = "NUM";
            inline static const char* TYPE_KEY = "TYPE";
            inline static const char* TYPE2_KEY = "TYPE2";
            inline static const char* MIN_TOL_KEY = "MIN_TOL";
            inline static const char* MAX_TOL_KEY = "MAX_TOL";
            inline static const char* BIT_KEY = "BIT";
            inline static const char* FINISH_SIZE_KEY = "FINISH_SIZE";
            inline static const char* DRILL_SIZE_KEY = "DRILL_SIZE";

            // Inherited via IProtoBuffable
            std::unique_ptr<Odb::Lib::Protobuf::ToolsFile::ToolsRecord> to_protobuf() const override;
            void from_protobuf(const Odb::Lib::Protobuf::ToolsFile::ToolsRecord& message) override;

            inline static const Utils::EnumMap<Type> typeMap{
                {
                    "PLATED",
                    "NON_PLATED",
                    "VIA"
                }
            };

            inline static const Utils::EnumMap<Type2> type2Map{
                {
                    "STANDARD",
                    "PRESS_FIT",
                    "PHOTO",
                    "LASER"
                }
            };

        };  // ToolsRecord

        // typedefs
        typedef std::map<unsigned int, std::shared_ptr<ToolsRecord>> ToolsMap;

        std::string GetUnits() const;
        double GetThickness() const;
        std::string GetUserParams() const;
        const ToolsMap& GetTools() const;

        bool Parse(std::filesystem::path directory);
        // Inherited via IStreamSaveable
        bool Save(std::ostream& os) override;

        // Inherited via IProtoBuffable
        std::unique_ptr<Odb::Lib::Protobuf::ToolsFile> to_protobuf() const override;
        void from_protobuf(const Odb::Lib::Protobuf::ToolsFile& message) override;

    private:
        std::filesystem::path m_directory;
        std::filesystem::path m_path;

        std::string m_units;
        double m_thickness;
        std::string m_user_params;
        ToolsMap m_toolsByNum;

        inline static const auto TOOLS_FILENAMES = { "tools" };

        inline static const char* UNITS_TOKEN = "UNITS";
        inline static const char* THICKNESS_TOKEN = "THICKNESS";
        inline static const char* USER_PARAMS_TOKEN = "USER_PARAMS";
    };
}

