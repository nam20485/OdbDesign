//
// Created by nmill on 10/13/2023.
//

#ifndef ODBDESIGN_MATRIXFILE_H
#define ODBDESIGN_MATRIXFILE_H

#include "OdbFile.h"
#include <filesystem>
#include <vector>
#include <memory>


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
            unsigned int id;
            std::string name;

            typedef std::vector<std::shared_ptr<StepRecord>> Vector;		
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

            enum class Polarity
			{
				Positive,
				Negative
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
                    
            /// @class struct for representing preferred layer color
            /// @brief each color value ranges from 0% - 100%            
            /// noPreference is true if the Odb++ file does not specify a color
            struct OdbRgbColor
            {
                bool noPreference;
                uint8_t red;
                uint8_t green;
                uint8_t blue;
            };

            typedef std::vector<std::shared_ptr<LayerRecord>> Vector;

            unsigned int row;
            Context context;
            Type type;
            std::string name;
            Polarity polarity;
            DielectricType dielectricType;
            std::string dielectricName;
            unsigned int cuTop;
            unsigned int cuBottom;
            unsigned int ref;
            std::string startName;
            std::string endName;
            std::string oldName;
            std::string addType;
            OdbRgbColor color;
            unsigned int id;

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

#endif //ODBDESIGN_MATRIXFILE_H
