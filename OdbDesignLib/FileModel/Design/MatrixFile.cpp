//
// Created by nmill on 10/13/2023.
//

#include "MatrixFile.h"
#include <fstream>
#include "str_utils.h"
#include <string>
#include "../../Constants.h"
#include <sstream>
#include "../parse_error.h"
#include <Logger.h>
#include "../invalid_odb_error.h"
#include "../../ProtoBuf/enums.pb.h"
#include "../../enums.h"
#include "../OdbFile.h"
#include <memory>
#include <ostream>

namespace Odb::Lib::FileModel::Design
{
    MatrixFile::~MatrixFile()
    {
        m_layerRecords.clear();
        m_stepRecords.clear();
    }

    const MatrixFile::LayerRecord::Vector& MatrixFile::GetLayerRecords() const
    { 
        return m_layerRecords;
    }

    const MatrixFile::StepRecord::Vector& MatrixFile::GetStepRecords() const
    { 
        return m_stepRecords;
    }

    bool MatrixFile::Parse(std::filesystem::path path)
	{
        std::ifstream matrixFile;
        int lineNumber = 0;
        std::string line;

        try
        {
            if (!OdbFile::Parse(path)) return false;

            auto matrixFilePath = path / "matrix";
            if (!std::filesystem::exists(matrixFilePath))
            {
                auto message = "matrix/matrix file does not exist: [" + matrixFilePath.string() + "]";
                throw invalid_odb_error(message.c_str());
            }

            matrixFile.open(matrixFilePath, std::ios::in);
            if (!matrixFile.is_open())
            {
                auto message = "unable to open matrix/matrix file: [" + matrixFilePath.string() + "]";
                throw invalid_odb_error(message.c_str());
            }

            std::shared_ptr<StepRecord> pCurrentStepRecord;
            std::shared_ptr<LayerRecord> pCurrentLayerRecord;
            bool openBraceFound = false;

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
                                if (attribute == StepRecord::COLUMN_KEY || attribute == "col")
                                {
                                    pCurrentStepRecord->column = std::stoi(value);
                                }
                                else if (attribute == StepRecord::NAME_KEY || attribute == "name")
                                {
                                    pCurrentStepRecord->name = value;
                                }
                                else if (attribute == StepRecord::ID_KEY || attribute == "id")
                                {
                                    pCurrentStepRecord->id = static_cast<unsigned int>(std::stoul(value));
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
                                    if (LayerRecord::typeMap.contains(value))
                                    {
                                        pCurrentLayerRecord->type = LayerRecord::typeMap.getValue(value);
                                    }                                        
                                    else
                                    {
                                        throw_parse_error(m_path, line, attribute, lineNumber);
                                    }
                                }
                                else if (attribute == LayerRecord::CONTEXT_KEY || attribute == "context")
                                {
                                    if (LayerRecord::contextMap.contains(value))
                                    {
										pCurrentLayerRecord->context = LayerRecord::contextMap.getValue(value);
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
                                else if (attribute == LayerRecord::POLARITY_KEY || attribute == "polarity")
                                {
                                    if (LayerRecord::polarityMap.contains(value))
                                    {
										pCurrentLayerRecord->polarity = LayerRecord::polarityMap.getValue(value);
									}
                                    else
                                    {
                                        throw_parse_error(m_path, line, attribute, lineNumber);
                                    }                                   
                                }
                                else if (attribute == LayerRecord::DIELECTRIC_TYPE_KEY || attribute == "dielectric_type")
                                {
                                    if (LayerRecord::dielectricTypeMap.contains(value))
                                    {
										pCurrentLayerRecord->dielectricType = LayerRecord::dielectricTypeMap.getValue(value);
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
                                else if (attribute == LayerRecord::FORM_KEY || attribute == "form")
                                {
                                    if (LayerRecord::formMap.contains(value))
                                    {
										pCurrentLayerRecord->form = LayerRecord::formMap.getValue(value);
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
                            logwarn("matrix/matrix file: no value for non-optional attribute: " + attribute);
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
        catch (invalid_odb_error& ioe)
        {
            parse_info pi(m_path, line, lineNumber);
            const auto m = pi.toString();
            logexception_msg(ioe, m);
            // cleanup file
            matrixFile.close();
            throw ioe;
        }

        return true;
	}

    /*static*/ bool MatrixFile::attributeValueIsOptional(const std::string& attribute)
    {
        for (const auto& optionalAttribute : OPTIONAL_ATTRIBUTES)
        {
            if (attribute == optionalAttribute)
            {
                return true;
            }
        }
        return false;
    }

    std::unique_ptr<Odb::Lib::Protobuf::MatrixFile> Odb::Lib::FileModel::Design::MatrixFile::to_protobuf() const
    {
        std::unique_ptr<Odb::Lib::Protobuf::MatrixFile> pMatrixFileMessage(new Odb::Lib::Protobuf::MatrixFile);
        for (const auto& stepRecord : m_stepRecords)
        {
            auto pStepRecordMessage = pMatrixFileMessage->add_steps();
            pStepRecordMessage->CopyFrom(*stepRecord->to_protobuf());
        }
        for (const auto& layerRecord : m_layerRecords)
        {
            auto pLayerRecordMessage = pMatrixFileMessage->add_layers();
            pLayerRecordMessage->CopyFrom(*layerRecord->to_protobuf());
        }
		return pMatrixFileMessage;
	
    }

    void Odb::Lib::FileModel::Design::MatrixFile::from_protobuf(const Odb::Lib::Protobuf::MatrixFile& message)
    {
        for (const auto& stepRecord : message.steps())
        {
			auto pStepRecord = std::make_shared<StepRecord>();
			pStepRecord->from_protobuf(stepRecord);
			m_stepRecords.push_back(pStepRecord);
		}
        for (const auto& layerRecord : message.layers())
        {
			auto pLayerRecord = std::make_shared<LayerRecord>();
			pLayerRecord->from_protobuf(layerRecord);
			m_layerRecords.push_back(pLayerRecord);
		}
    }

    bool MatrixFile::Save(std::ostream& os)
    {
        for (const auto& stepRecord : m_stepRecords)
        {
			os << StepRecord::RECORD_TOKEN << " " << Constants::ARRAY_RECORD_OPEN_TOKEN << std::endl;

			os << '\t' << StepRecord::COLUMN_KEY << "=" << stepRecord->column << std::endl;
            os << '\t' << StepRecord::NAME_KEY << "=" << stepRecord->name << std::endl;
            os << '\t' << StepRecord::ID_KEY << "=" << stepRecord->id << std::endl;

			os << Constants::ARRAY_RECORD_CLOSE_TOKEN << std::endl;
            os << std::endl;
		}        

        for (const auto& layerRecord : m_layerRecords)
        {
            os << LayerRecord::RECORD_TOKEN << " " << Constants::ARRAY_RECORD_OPEN_TOKEN << std::endl;

            os << '\t' << LayerRecord::ROW_KEY << "=" << layerRecord->row << std::endl;
            os << '\t' << LayerRecord::CONTEXT_KEY << "=" << LayerRecord::contextMap.getValue(layerRecord->context) << std::endl;
            os << '\t' << LayerRecord::TYPE_KEY << "=" << LayerRecord::typeMap.getValue(layerRecord->type) << std::endl;
            os << '\t' << LayerRecord::NAME_KEY << "=" << layerRecord->name << std::endl;                      
            os << '\t' << LayerRecord::POLARITY_KEY << "=" << LayerRecord::polarityMap.getValue(layerRecord->polarity) << std::endl;
            os << '\t' << LayerRecord::FORM_KEY << "=" << LayerRecord::formMap.getValue(layerRecord->form) << std::endl;
            os << '\t' << LayerRecord::DIELECTRIC_TYPE_KEY << "=" << LayerRecord::dielectricTypeMap.getValue(layerRecord->dielectricType) << std::endl;
            os << '\t' << LayerRecord::DIELECTRIC_NAME_KEY << "=" << layerRecord->dielectricName << std::endl;            
            os << '\t' << LayerRecord::CU_TOP_KEY << "=";
            if (layerRecord->cuTop != (unsigned int)-1)
            {
                os << layerRecord->cuTop;
            }    
            os << std::endl;
            os << '\t' << LayerRecord::CU_BOTTOM_KEY << "=";
            if (layerRecord->cuBottom != (unsigned int)-1)
            {
				os << layerRecord->cuBottom;
			}
            os << std::endl;
            os << '\t' << LayerRecord::REF_KEY << "=";
            if (layerRecord->ref != (unsigned int)-1)
            {
                os << layerRecord->ref;
            }
            os << std::endl;
            os << '\t' << LayerRecord::START_NAME_KEY << "=" << layerRecord->startName << std::endl;
            os << '\t' << LayerRecord::END_NAME_KEY << "=" << layerRecord->endName << std::endl;
            os << '\t' << LayerRecord::OLD_NAME_KEY << "=" << layerRecord->oldName << std::endl;
            os << '\t' << LayerRecord::ADD_TYPE_KEY << "=" << layerRecord->addType << std::endl;
            os << '\t' << LayerRecord::COLOR_KEY << "=" << layerRecord->color.to_string() << std::endl;
            os << '\t' << LayerRecord::ID_KEY << "=" << layerRecord->id << std::endl;
            
            os << Constants::ARRAY_RECORD_CLOSE_TOKEN << std::endl;            
            os << std::endl;
        }

        return true;
    }
    
    std::unique_ptr<Odb::Lib::Protobuf::MatrixFile::StepRecord> Odb::Lib::FileModel::Design::MatrixFile::StepRecord::to_protobuf() const
    {
		std::unique_ptr<Odb::Lib::Protobuf::MatrixFile::StepRecord> pStepRecordMessage(new Odb::Lib::Protobuf::MatrixFile::StepRecord);
		pStepRecordMessage->set_column(column);
		pStepRecordMessage->set_name(name);
		pStepRecordMessage->set_id(id);
		return pStepRecordMessage;
	}

    void Odb::Lib::FileModel::Design::MatrixFile::StepRecord::from_protobuf(const Odb::Lib::Protobuf::MatrixFile::StepRecord& message)
    {
        column = message.column();
        name = message.name();
        id = message.id();
    }

    std::unique_ptr<Odb::Lib::Protobuf::MatrixFile::LayerRecord> Odb::Lib::FileModel::Design::MatrixFile::LayerRecord::to_protobuf() const
    {
        std::unique_ptr<Odb::Lib::Protobuf::MatrixFile::LayerRecord> pLayerRecordMessage(new Odb::Lib::Protobuf::MatrixFile::LayerRecord);
        pLayerRecordMessage->set_addtype(addType);
        pLayerRecordMessage->set_context(static_cast<Odb::Lib::Protobuf::MatrixFile::LayerRecord::Context>(context));
        pLayerRecordMessage->set_cubottom(cuBottom);
        pLayerRecordMessage->set_cutop(cuTop);
        pLayerRecordMessage->set_dielectricname(dielectricName);
        pLayerRecordMessage->set_dielectrictype(static_cast<Odb::Lib::Protobuf::MatrixFile::LayerRecord::DielectricType>(dielectricType));
        pLayerRecordMessage->set_endname(endName);
        pLayerRecordMessage->set_form(static_cast<Odb::Lib::Protobuf::MatrixFile::LayerRecord::Form>(form));
        pLayerRecordMessage->set_id(id);
        pLayerRecordMessage->set_name(name);
        pLayerRecordMessage->set_oldname(oldName);
        pLayerRecordMessage->set_polarity(static_cast<Odb::Lib::Protobuf::Polarity>(polarity));
        pLayerRecordMessage->set_ref(ref);
        pLayerRecordMessage->set_row(row);
        pLayerRecordMessage->set_startname(startName);
        pLayerRecordMessage->set_type(static_cast<Odb::Lib::Protobuf::MatrixFile::LayerRecord::Type>(type));
        //pLayerRecordMessage->mutable_color()->set_r(color.r);
        //pLayerRecordMessage->mutable_color()->set_g(color.g);
        //pLayerRecordMessage->mutable_color()->set_b(color.b);


        return pLayerRecordMessage;
    }       

    void Odb::Lib::FileModel::Design::MatrixFile::LayerRecord::from_protobuf(const Odb::Lib::Protobuf::MatrixFile::LayerRecord& message)
    {
        addType = message.addtype();
		context = static_cast<Context>(message.context());
		cuBottom = message.cubottom();
		cuTop = message.cutop();
		dielectricName = message.dielectricname();
		dielectricType = static_cast<DielectricType>(message.dielectrictype());
		endName = message.endname();
		form = static_cast<Form>(message.form());
		id = message.id();
		name = message.name();
		oldName = message.oldname();
		polarity = static_cast<Polarity>(message.polarity());
		ref = message.ref();
		row = message.row();
		startName = message.startname();
		type = static_cast<Type>(message.type());
		//color.r = message.color().r();
		//color.g = message.color().g();
		//color.b = message.color().b();
    }   
}