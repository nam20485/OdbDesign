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


namespace Odb::Lib::FileModel::Design
{
    class MatrixFile : public OdbFile
    {
    public:
        MatrixFile() = default;
        ~MatrixFile() = default;      

        struct StepRecord
        {
            unsigned int column;
            unsigned int id = (unsigned int) -1;
            std::string name;

            typedef std::vector<std::shared_ptr<StepRecord>> Vector;	

            inline static const char* RECORD_TOKEN = "STEP";
        };

        struct LayerRecord
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
                None,
                Prepreg,
                Core
            };

            enum class Form
            {
                Rigid,
                Flex
            };
                    
            

            typedef std::vector<std::shared_ptr<LayerRecord>> Vector;

            int row;
            Context context;
            Type type;
            std::string name;
            Polarity polarity;
            DielectricType dielectricType = DielectricType::None;
            std::string dielectricName;
            Form form = Form::Rigid;
            unsigned int cuTop = (unsigned int) -1;
            unsigned int cuBottom = (unsigned int) -1;
            unsigned int ref = (unsigned int) -1;
            std::string startName;
            std::string endName;
            std::string oldName;
            std::string addType;
            RgbColor color{"0"};
            unsigned int id = (unsigned int) -1;

            inline static const char* RECORD_TOKEN = "LAYER";

        };

        const LayerRecord::Vector& GetLayerRecords() const { return m_layerRecords; }
        const StepRecord::Vector& GetStepRecords() const { return m_stepRecords; }

        // Inherited via OdbFile
        bool Parse(std::filesystem::path path) override;

    private:
        LayerRecord::Vector m_layerRecords;
        StepRecord::Vector m_stepRecords;

    };
}
