//
// Created by nmill on 10/13/2023.
//

#include "MatrixFile.h"
#include <fstream>
#include "str_trim.h"
#include <string>
#include "Constants.h"

namespace Odb::Lib::FileModel::Design
{
	bool MatrixFile::Parse(std::filesystem::path path)
	{
		if (!OdbFile::Parse(path)) return false;

        auto matrixFilePath = path / "matrix";
        if (!std::filesystem::exists(matrixFilePath)) return false;

        std::ifstream matrixFile;
        matrixFile.open(matrixFilePath, std::ios::in);
        if (!matrixFile.is_open()) return false;

        std::shared_ptr<StepRecord> pCurrentStepRecord;
        std::shared_ptr<LayerRecord> pCurrentLayerRecord;
        bool openBraceFound = false;

        std::string line;
        while (std::getline(matrixFile, line))
        {
            // trim whitespace from beginning and end of line
            Utils::str_trim(line);
            if (!line.empty())
            {
                std::stringstream lineStream(line);
                if (line.find(Constants::COMMENT_TOKEN) == 0)
                {
                    // comment line
                }
                else if (line.find(StepRecord::RECORD_TOKEN))
                {
                    std::string token;
                    if (!(lineStream >> token)) return false;
                    if (token != StepRecord::RECORD_TOKEN) return false;

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
                else if (line.find(LayerRecord::RECORD_TOKEN))
                {
                    std::string token;
                    if (!(lineStream >> token)) return false;
                    if (token != LayerRecord::RECORD_TOKEN) return false;

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
                    if (pCurrentStepRecord == nullptr && pCurrentLayerRecord == nullptr) return false;
                    // found another open brace while still parsing after the previous record's open brace
                    if (openBraceFound) return false;

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
                        return false;
                    }
                }                
                else
                {
                    // it should be a name-value pair line (i.e. element of a step or layer array)
                    std::string attribute;
                    std::string value;

                    if (!std::getline(lineStream, attribute, '=')) return false;
                    if (!std::getline(lineStream, value)) return false;

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
                            pCurrentStepRecord->id = (unsigned int) std::stoul(value);
                        }
                        else
                        {
                            return false;
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
							else if (value == "SOLDE_RMASK")
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
                                return false;
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
                                return false;
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
                                pCurrentLayerRecord->polarity = LayerRecord::Polarity::Positive;
                            }
                            else if (value == "NEGATIVE")
                            {
                                pCurrentLayerRecord->polarity = LayerRecord::Polarity::Negative;
                            }
                            else
                            {
                                return false;
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
								return false;
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
								return false;
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
                            return false;
                        }
					}
                    else
                    {
                        // found a name-value pair but aren't currently parsing a step or layer array record
                        return false;
                    }                    
                }
            }
        }

        return true;
	}
}