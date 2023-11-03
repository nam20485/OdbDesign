#include "MatrixFile.h"
#include "MatrixFile.h"
#include "MatrixFile.h"
#include "MatrixFile.h"
#include "MatrixFile.h"
//
// Created by nmill on 10/13/2023.
//

#include "MatrixFile.h"
#include <fstream>
#include "str_trim.h"
#include <string>
#include "../../Constants.h"
#include <sstream>
#include "../parse_error.h"
#include <Logger.h>

namespace Odb::Lib::FileModel::Design
{
    inline const MatrixFile::LayerRecord::Vector& MatrixFile::GetLayerRecords() const
    { 
        return m_layerRecords;
    }

    inline const MatrixFile::StepRecord::Vector& MatrixFile::GetStepRecords() const
    { 
        return m_stepRecords;
    }

    bool MatrixFile::Parse(std::filesystem::path path)
	{
        std::ifstream matrixFile;

        try
        {
            if (!OdbFile::Parse(path)) return false;

            auto matrixFilePath = path / "matrix";
            if (!std::filesystem::exists(matrixFilePath))
            {
                auto message = "matrix/matrix file does not exist: [" + matrixFilePath.string() + "]";
                throw std::exception(message.c_str());
            }

            matrixFile.open(matrixFilePath, std::ios::in);
            if (!matrixFile.is_open())
            {
                auto message = "unable to open matrix/matrix file: [" + matrixFilePath.string() + "]";
                throw std::exception(message.c_str());
            }

            std::shared_ptr<StepRecord> pCurrentStepRecord;
            std::shared_ptr<LayerRecord> pCurrentLayerRecord;
            bool openBraceFound = false;

            int lineNumber = 0;
            std::string line;
            while (std::getline(matrixFile, line))
            {
                lineNumber++;
                // trim whitespace from beginning and end of line
                Utils::str_trim(line);
                if (!line.empty())
                {
                    std::stringstream lineStream(line);
                    if (line.find(Constants::COMMENT_TOKEN) == 0)
                    {
                        // comment line
                    }
                    else if (line.find(StepRecord::RECORD_TOKEN) == 0)
                    {
                        std::string token;
                        if (!(lineStream >> token))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (token != StepRecord::RECORD_TOKEN)
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (lineStream >> token)
                        {
                            // open brace is at the end on the same line as the step array record open token
                            if (token == Constants::ARRAY_RECORD_OPEN_TOKEN)
                            {
                                openBraceFound = true;

                                // TODO: finish any existing previous step record
                                // (same code as finding a close brace on an empty line)

                                // open a new STEP array record
                                pCurrentStepRecord = std::make_shared<StepRecord>();
                            }
                        }
                    }
                    else if (line.find(LayerRecord::RECORD_TOKEN) == 0)
                    {
                        std::string token;
                        if (!(lineStream >> token))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (token != LayerRecord::RECORD_TOKEN)
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (lineStream >> token)
                        {
                            // open brace is on same line as layer record open token
                            if (token == Constants::ARRAY_RECORD_OPEN_TOKEN)
                            {
                                openBraceFound = true;

                                // TODO: finish any existing previous layer or step record
                                // (same code as finding a close brace on an empty line)

                                // open a new LAYER array record
                                pCurrentLayerRecord = std::make_shared<LayerRecord>();
                            }
                        }
                    }
                    else if (line.find(Constants::ARRAY_RECORD_OPEN_TOKEN) == 0)
                    {
                        // TODO: how to determine if you are opening a step or layer array record?
                        // (maybe a boolean flag? stepArrayOpen = true/false)
                        // no current opening of a layer or step array record found yet
                        if (pCurrentStepRecord == nullptr && pCurrentLayerRecord == nullptr)
                        {
                            throw_parse_error(m_path, line, "", lineNumber);
                        }

                        // found another open brace while still parsing after the previous record's open brace
                        if (openBraceFound)
                        {
                            throw_parse_error(m_path, line, "", lineNumber);
                        }

                        openBraceFound = true;

                        // TODO: finish any existing previous layer or step record
                        // (same code as finding a close brace on an empty line)

                        // open a new LAYER array record
                        //pCurrentLayerRecord = std::make_shared<LayerRecord>();
                    }
                    else if (line.find(Constants::ARRAY_RECORD_CLOSE_TOKEN) == 0)
                    {
                        if (pCurrentStepRecord != nullptr && openBraceFound)
                        {
                            m_stepRecords.push_back(pCurrentStepRecord);
                            pCurrentStepRecord.reset();
                            openBraceFound = false;
                        }
                        else if (pCurrentLayerRecord != nullptr && openBraceFound)
                        {
                            m_layerRecords.push_back(pCurrentLayerRecord);
                            pCurrentLayerRecord.reset();
                            openBraceFound = false;
                        }
                        else
                        {
                            // found a close brace but aren't currently parsing a step or layer array record
                            throw_parse_error(m_path, line, "", lineNumber);
                        }
                    }
                    else
                    {
                        // it should be a name-value pair line (i.e. element of a step or layer array)
                        std::string attribute;
                        std::string value;

                        if (!std::getline(lineStream, attribute, '='))
                        {
                            throw_parse_error(m_path, line, attribute, lineNumber);
                        }

                        if (std::getline(lineStream, value))
                        {
                            Utils::str_trim(attribute);
                            Utils::str_trim(value);

                            if (pCurrentStepRecord != nullptr && openBraceFound)
                            {
                                if (attribute == "COL" || attribute == "col")
                                {
                                    pCurrentStepRecord->column = std::stoi(value);
                                }
                                else if (attribute == "NAME" || attribute == "name")
                                {
                                    pCurrentStepRecord->name = value;
                                }
                                else if (attribute == "ID" || attribute == "id")
                                {
                                    pCurrentStepRecord->id = (unsigned int)std::stoul(value);
                                }
                                else
                                {
                                    throw_parse_error(m_path, line, attribute, lineNumber);
                                }
                            }
                            else if (pCurrentLayerRecord != nullptr && openBraceFound)
                            {
                                if (attribute == "ROW" || attribute == "row")
                                {
                                    pCurrentLayerRecord->row = std::stoi(value);
                                }
                                else if (attribute == "NAME" || attribute == "name")
                                {
                                    pCurrentLayerRecord->name = value;
                                }
                                else if (attribute == "ID" || attribute == "id")
                                {
                                    pCurrentLayerRecord->id = (unsigned int)std::stoul(value);
                                }
                                else if (attribute == "TYPE" || attribute == "type")
                                {
                                    if (value == "SIGNAL")
                                    {
                                        pCurrentLayerRecord->type = LayerRecord::Type::Signal;
                                    }
                                    else if (value == "POWER_GROUND")
                                    {
                                        pCurrentLayerRecord->type = LayerRecord::Type::PowerGround;
                                    }
                                    else if (value == "DIELECTRIC")
                                    {
                                        pCurrentLayerRecord->type = LayerRecord::Type::Dielectric;
                                    }
                                    else if (value == "MIXED")
                                    {
                                        pCurrentLayerRecord->type = LayerRecord::Type::Mixed;
                                    }
                                    else if (value == "SOLDER_MASK")
                                    {
                                        pCurrentLayerRecord->type = LayerRecord::Type::SolderMask;
                                    }
                                    else if (value == "SOLDER_PASTE")
                                    {
                                        pCurrentLayerRecord->type = LayerRecord::Type::SolderPaste;
                                    }
                                    else if (value == "SILK_SCREEN")
                                    {
                                        pCurrentLayerRecord->type = LayerRecord::Type::SilkScreen;
                                    }
                                    else if (value == "DRILL")
                                    {
                                        pCurrentLayerRecord->type = LayerRecord::Type::Drill;
                                    }
                                    else if (value == "ROUT")
                                    {
                                        pCurrentLayerRecord->type = LayerRecord::Type::Rout;
                                    }
                                    else if (value == "DOCUMENT")
                                    {
                                        pCurrentLayerRecord->type = LayerRecord::Type::Document;
                                    }
                                    else if (value == "COMPONENT")
                                    {
                                        pCurrentLayerRecord->type = LayerRecord::Type::Component;
                                    }
                                    else if (value == "MASK")
                                    {
                                        pCurrentLayerRecord->type = LayerRecord::Type::Mask;
                                    }
                                    else if (value == "CONDUCTIVE_PASTE")
                                    {
                                        pCurrentLayerRecord->type = LayerRecord::Type::ConductivePaste;
                                    }
                                    else
                                    {
                                        throw_parse_error(m_path, line, attribute, lineNumber);
                                    }
                                }
                                else if (attribute == "CONTEXT" || attribute == "context")
                                {
                                    if (value == "BOARD")
                                    {
                                        pCurrentLayerRecord->context = LayerRecord::Context::Board;
                                    }
                                    else if (value == "MISC")
                                    {
                                        pCurrentLayerRecord->context = LayerRecord::Context::Misc;
                                    }
                                    else
                                    {
                                        throw_parse_error(m_path, line, attribute, lineNumber);
                                    }
                                }
                                else if (attribute == "OLD_NAME" || attribute == "old_name")
                                {
                                    pCurrentLayerRecord->oldName = value;
                                }
                                else if (attribute == "POLARITY" || attribute == "polarity")
                                {
                                    if (value == "POSITIVE")
                                    {
                                        pCurrentLayerRecord->polarity = Polarity::Positive;
                                    }
                                    else if (value == "NEGATIVE")
                                    {
                                        pCurrentLayerRecord->polarity = Polarity::Negative;
                                    }
                                    else
                                    {
                                        throw_parse_error(m_path, line, attribute, lineNumber);
                                    }
                                }
                                else if (attribute == "DIELECTRIC_TYPE" || attribute == "dielectric_type")
                                {
                                    if (value == "NONE")
                                    {
                                        pCurrentLayerRecord->dielectricType = LayerRecord::DielectricType::None;
                                    }
                                    else if (value == "PREPREG")
                                    {
                                        pCurrentLayerRecord->dielectricType = LayerRecord::DielectricType::Prepreg;
                                    }
                                    else if (value == "CORE")
                                    {
                                        pCurrentLayerRecord->dielectricType = LayerRecord::DielectricType::Core;
                                    }
                                    else
                                    {
                                        throw_parse_error(m_path, line, attribute, lineNumber);
                                    }
                                }
                                else if (attribute == "DIELECTRIC_NAME" || attribute == "dielectric_name")
                                {
                                    pCurrentLayerRecord->dielectricName = value;
                                }
                                else if (attribute == "FORM" || attribute == "form")
                                {
                                    if (value == "RIGID")
                                    {
                                        pCurrentLayerRecord->form = LayerRecord::Form::Rigid;
                                    }
                                    else if (value == "FLEX")
                                    {
                                        pCurrentLayerRecord->form = LayerRecord::Form::Flex;
                                    }
                                    else
                                    {
                                        throw_parse_error(m_path, line, attribute, lineNumber);
                                    }
                                }
                                else if (attribute == "CU_TOP" || attribute == "cu_top")
                                {
                                    pCurrentLayerRecord->cuTop = std::stoul(value);
                                }
                                else if (attribute == "CU_TOP" || attribute == "cu_top")
                                {
                                    pCurrentLayerRecord->cuTop = std::stoul(value);
                                }
                                else if (attribute == "CU_BOTTOM" || attribute == "cu_bottom")
                                {
                                    pCurrentLayerRecord->cuBottom = std::stoul(value);
                                }
                                else if (attribute == "REF" || attribute == "ref")
                                {
                                    pCurrentLayerRecord->ref = std::stoul(value);
                                }
                                else if (attribute == "START_NAME" || attribute == "start_name")
                                {
                                    pCurrentLayerRecord->startName = value;
                                }
                                else if (attribute == "END_NAME" || attribute == "end_name")
                                {
                                    pCurrentLayerRecord->endName = value;
                                }
                                else if (attribute == "ADD_TYPE" || attribute == "add_type")
                                {
                                    pCurrentLayerRecord->addType = value;
                                }
                                else if (attribute == "COLOR" || attribute == "color")
                                {
                                    if (!pCurrentLayerRecord->color.from_string(value)) return false;
                                }
                                else
                                {
                                    throw_parse_error(m_path, line, attribute, lineNumber);
                                }
                            }
                            else
                            {
                                // found a name-value pair but aren't currently parsing a step or layer array record
                                //return false;
                                throw_parse_error(m_path, line, attribute, lineNumber);                                
                            }
                        }
                        else if (!attributeValueIsOptional(attribute))
                        {
                            logwarn("matrix/matrix file: no value for attribute: " + attribute);
                        }                        
                    }
                }
            }

            matrixFile.close();
        }
        catch (parse_error& pe)
        {
            auto m = pe.toString("Parse Error:");
            logerror(m);
            // cleanup file
            matrixFile.close();
            throw pe;
        }
        catch (std::exception& e)
        {
            logexception(e);
            // cleanup file
            matrixFile.close();
            throw e;
        }


        return true;
	}

    /*static*/ bool MatrixFile::attributeValueIsOptional(const std::string& attribute)
    {
        auto attributeIsOptional = false;
        for (const auto& optionalAttribute : OPTIONAL_ATTRIBUTES)
        {
            if (attribute == optionalAttribute)
            {
                attributeIsOptional = true;
            }
        }
        return attributeIsOptional;
    }
}