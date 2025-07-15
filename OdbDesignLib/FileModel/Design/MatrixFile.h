#pragma once

//
// Created by nmill on 10/13/2023.
//

#include "../OdbFile.h"
#include <filesystem>
#include <vector>
#include <memory>
#include "RgbColor.h"
#include "../../enums.h"
#include "../../IProtoBuffable.h"
#include "../../ProtoBuf/matrixfile.pb.h"
#include "../IStreamSaveable.h"
#include "EnumMap.h"
#include <string>
#include <iostream>
#include "../../odbdesign_export.h"


namespace Odb::Lib::FileModel::Design
{
    class ODBDESIGN_EXPORT MatrixFile : public OdbFile, public IProtoBuffable<Odb::Lib::Protobuf::MatrixFile>, public IStreamSaveable
    {
    public:
        ~MatrixFile();

        struct StepRecord : public IProtoBuffable<Odb::Lib::Protobuf::MatrixFile::StepRecord>
        {
            unsigned int column;
            unsigned int id = (unsigned int)-1;
            std::string name;

            typedef std::vector<std::shared_ptr<StepRecord>> Vector;

            inline static const char* RECORD_TOKEN = "STEP";
            inline static const char* COLUMN_KEY = "COL";
            inline static const char* NAME_KEY = "NAME";
            inline static const char* ID_KEY = "ID";

            // Inherited via IProtoBuffable
            std::unique_ptr<Odb::Lib::Protobuf::MatrixFile::StepRecord> to_protobuf() const override;
            void from_protobuf(const Odb::Lib::Protobuf::MatrixFile::StepRecord& message) override;
        };

        struct LayerRecord : public IProtoBuffable<Odb::Lib::Protobuf::MatrixFile::LayerRecord>
        {
            enum class Type
            {
                Signal,
                PowerGround,
                Dielectric,
                Mixed,
                SolderMask,
                SolderPaste,
                SilkScreen,
                Drill,
                Rout,
                Document,
                Component,
                Mask,
                ConductivePaste,
            };

            enum class Context
            {
                Board,
                Misc
            };

            enum class DielectricType
            {
                NotSet,
                None,
                Prepreg,
                Core
            };

            enum class Form
            {
                NotSet,
                Rigid,
                Flex
            };

            typedef std::vector<std::shared_ptr<LayerRecord>> Vector;

            int row;
            Context context;
            Type type;
            std::string name;
            Polarity polarity;
            DielectricType dielectricType = DielectricType::NotSet;
            std::string dielectricName;
            Form form = Form::NotSet;
            unsigned int cuTop = (unsigned int)-1;
            unsigned int cuBottom = (unsigned int)-1;
            unsigned int ref = (unsigned int)-1;
            std::string startName;
            std::string endName;
            std::string oldName;
            std::string addType;
            RgbColor color{ "0" };
            unsigned int id = (unsigned int)-1;

            inline static const char* RECORD_TOKEN = "LAYER";

            inline static const char* ROW_KEY = "ROW";
            inline static const char* CONTEXT_KEY = "CONTEXT";
            inline static const char* TYPE_KEY = "TYPE";
            inline static const char* NAME_KEY = "NAME";
            inline static const char* POLARITY_KEY = "POLARITY";
            inline static const char* DIELECTRIC_TYPE_KEY = "DIELECTRIC_TYPE";
            inline static const char* DIELECTRIC_NAME_KEY = "DIELECTRIC_NAME";
            inline static const char* FORM_KEY = "FORM";
            inline static const char* CU_TOP_KEY = "CU_TOP";
            inline static const char* CU_BOTTOM_KEY = "CU_BOTTOM";
            inline static const char* REF_KEY = "REF";
            inline static const char* START_NAME_KEY = "START_NAME";
            inline static const char* END_NAME_KEY = "END_NAME";
            inline static const char* OLD_NAME_KEY = "OLD_NAME";
            inline static const char* ADD_TYPE_KEY = "ADD_TYPE";
            inline static const char* COLOR_KEY = "COLOR";
            inline static const char* ID_KEY = "ID";

            // Inherited via IProtoBuffable
            std::unique_ptr<Odb::Lib::Protobuf::MatrixFile::LayerRecord> to_protobuf() const override;
            void from_protobuf(const Odb::Lib::Protobuf::MatrixFile::LayerRecord& message) override;

            inline static const Utils::EnumMap<Type> typeMap {
                {
				    "SIGNAL",
				    "POWER_GROUND",
				    "DIELECTRIC",
				    "MIXED",
                    "SOLDER_MASK",
                    "SOLDER_PASTE",
                    "SILK_SCREEN",
				    "DRILL",
				    "ROUT",
				    "DOCUMENT",
				    "COMPONENT",
				    "MASK",
				    "CONDUCTIVE_PASTE"
                }
            };

            inline static const Utils::EnumMap<Context> contextMap{
               {
                   "BOARD",
                   "MISC"
               }
            };

            inline static const Utils::EnumMap<DielectricType> dielectricTypeMap{
                {
                    "",
                    "NONE",
                    "PREPREG",
                    "CORE"
                }
            };

            inline static const Utils::EnumMap<Form> formMap{
                {
                    "",
                    "RIGID",
                    "FLEX"
                }
            };

            inline static const Utils::EnumMap<Polarity> polarityMap{
                {
					"POSITIVE",
					"NEGATIVE"
				}
			};
        };

        const LayerRecord::Vector& GetLayerRecords() const;
        const StepRecord::Vector& GetStepRecords() const;

        // Inherited via OdbFile
        bool Parse(std::filesystem::path path) override;
        // Inherited via IStreamSaveable
        bool Save(std::ostream& os) override;

        static inline bool attributeValueIsOptional(const std::string& attribute);

        // Inherited via IProtoBuffable
        std::unique_ptr<Odb::Lib::Protobuf::MatrixFile> to_protobuf() const override;
        void from_protobuf(const Odb::Lib::Protobuf::MatrixFile& message) override;

    private:
        LayerRecord::Vector m_layerRecords;
        StepRecord::Vector m_stepRecords;

        constexpr inline static const char* OPTIONAL_ATTRIBUTES[] =
        {
            "OLD_NAME",
            "old_name",
            "START_NAME",
            "start_name",
            "END_NAME",
            "end_name",
            "ADD_TYPE",
            "ID",
            "DIELECTRIC_TYPE",
            "DIELECTRIC_NAME",
            "FORM",
            "CU_TOP",
            "CU_BOTTOM",
            "REF",
            "COLOR",
        };      
    };
}
