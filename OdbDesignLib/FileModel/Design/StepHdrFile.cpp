#include "StepHdrFile.h"
#include "../invalid_odb_error.h"
#include "str_utils.h"
#include "../../Constants.h"
#include "../parse_error.h"
#include <Logger.h>

namespace Odb::Lib::FileModel::Design
{
	StepHdrFile::~StepHdrFile()
	{
		m_stepRepeatRecords.clear();
	}

	bool StepHdrFile::Parse(std::filesystem::path path)
	{
        std::ifstream stepHdrFile;
        int lineNumber = 0;
        std::string line;

        try
        {
            if (!OdbFile::Parse(path)) return false;

            auto stepHdrFilePath = path / "stephdr";
            if (!std::filesystem::exists(stepHdrFilePath))
            {
                auto message = "stephdr file does not exist: [" + stepHdrFilePath.string() + "]";
                throw invalid_odb_error(message.c_str());
            }

            stepHdrFile.open(stepHdrFilePath, std::ios::in);
            if (!stepHdrFile.is_open())
            {
                auto message = "unable to open stephdr file: [" + stepHdrFilePath.string() + "]";
                throw invalid_odb_error(message.c_str());
            }

            m_path = stepHdrFilePath;

            std::shared_ptr<StepRepeatRecord> pCurrentStepRepeatRecord;
            bool openBraceFound = false;

            while (std::getline(stepHdrFile, line))
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
                    else if (line.find(StepRepeatRecord::ARRAY_HEADER_TOKEN) == 0)
                    {
                        std::string token;
                        if (!(lineStream >> token))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (token != StepRepeatRecord::ARRAY_HEADER_TOKEN)
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

                                // open a new STEP-REPEAT array record
                                pCurrentStepRepeatRecord = std::make_shared<StepRepeatRecord>();
                            }
                        }
                    }                    
                    else if (line.find(Constants::ARRAY_RECORD_OPEN_TOKEN) == 0)
                    {
                        // TODO: how to determine if you are opening a step or layer array record?
                        // (maybe a boolean flag? stepArrayOpen = true/false)
                        // no current opening of a layer or step array record found yet
                        if (pCurrentStepRepeatRecord == nullptr)
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
                        if (pCurrentStepRepeatRecord != nullptr && openBraceFound)
                        {
                            m_stepRepeatRecords.push_back(pCurrentStepRepeatRecord);
                            pCurrentStepRepeatRecord.reset();
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

                            if (pCurrentStepRepeatRecord != nullptr && openBraceFound)
                            {
                                if (attribute == "NAME" || attribute == "name")
                                {
                                    pCurrentStepRepeatRecord->name = value;
                                }
                                else if (attribute == "X" || attribute == "x")
                                {
                                    pCurrentStepRepeatRecord->x = std::stof(value);
                                }
                                else if (attribute == "Y" || attribute == "y")
								{
									pCurrentStepRepeatRecord->y = std::stof(value);
								}
								else if (attribute == "DX" || attribute == "dx")
								{
									pCurrentStepRepeatRecord->dx = std::stof(value);
								}
								else if (attribute == "DY" || attribute == "dy")
								{
									pCurrentStepRepeatRecord->dy = std::stof(value);
								}
								else if (attribute == "NX" || attribute == "nx")
								{
									pCurrentStepRepeatRecord->nx = std::stoi(value);
								}
								else if (attribute == "NY" || attribute == "ny")
								{
									pCurrentStepRepeatRecord->ny = std::stoi(value);
								}
								else if (attribute == "ANGLE" || attribute == "angle")
								{
									pCurrentStepRepeatRecord->angle = std::stof(value);
								}
                                else if (attribute == "FLIP" || attribute == "flip")
                                {
                                    if (value == "YES" || value == "yes")
                                    {
										pCurrentStepRepeatRecord->flip = true;
									}
                                    else if (value == "NO" || value == "no")
                                    {
										pCurrentStepRepeatRecord->flip = false;
									}
                                    else
                                    {
										throw_parse_error(m_path, line, attribute, lineNumber);
									}
                                }
                                else if (attribute == "MIRROR" || attribute == "mirror")
                                {
                                    if (value == "YES" || value == "yes")
                                    {
                                        pCurrentStepRepeatRecord->mirror = true;
                                    }
                                    else if (value == "NO" || value == "no")
                                    {
                                        pCurrentStepRepeatRecord->mirror = false;
                                    }
                                    else
                                    {
                                        throw_parse_error(m_path, line, attribute, lineNumber);
                                    }
                                }
                                else
                                {
                                    throw_parse_error(m_path, line, attribute, lineNumber);
                                }
                            }
                            else
                            {                               
                                if (attribute == "X_DATUM" || attribute == "x_datum")
                                {
                                    xDatum = std::stof(value);
                                }
                                else if (attribute == "Y_DATUM" || attribute == "y_datum")
                                {
                                    yDatum = stof(value);
                                }
                                else if (attribute == "ID" || attribute == "id")
								{
									id = (unsigned int)std::stoul(value);
								}
								else if (attribute == "X_ORIGIN" || attribute == "x_origin")
								{
									xOrigin = std::stof(value);
								}
								else if (attribute == "Y_ORIGIN" || attribute == "y_origin")
								{
									yOrigin = std::stof(value);
								}
								else if (attribute == "TOP_ACTIVE" || attribute == "top_active")
								{
									topActive = std::stof(value);
								}
								else if (attribute == "BOTTOM_ACTIVE" || attribute == "bottom_active")
								{
									bottomActive = std::stof(value);
								}
								else if (attribute == "RIGHT_ACTIVE" || attribute == "right_active")
								{
									rightActive = std::stof(value);
								}
								else if (attribute == "LEFT_ACTIVE" || attribute == "left_active")
								{
									leftActive = std::stof(value);
								}
								else if (attribute == "AFFECTING_BOM" || attribute == "affecting_bom")
								{
                                    affectingBom = value;
								}
								else if (attribute == "AFFECTING_BOM_CHANGED" || attribute == "affecting_bom_changed")
								{
									affectingBomChanged = std::stoi(value);
								}
                                else if (attribute == "UNITS" || attribute == "units")
                                {
                                    m_units = value;
                                }
                                else if (attribute.find("ONLINE_") == 0)
                                {
                                    m_onlineValues[attribute] = value;
                                }
                                else
                                {
                                    throw_parse_error(m_path, line, attribute, lineNumber);
                                }
                            }
                        }
                        //else if (!attributeValueIsOptional(attribute))
                        //{
                        //    logwarn("matrix/matrix file: no value for non-optional attribute: " + attribute);
                        //}
                    }
                }
            }

            stepHdrFile.close();
        }
        catch (parse_error& pe)
        {
            auto m = pe.toString("Parse Error:");
            logerror(m);
            // cleanup file
            stepHdrFile.close();
            throw pe;
        }
        catch (invalid_odb_error& ioe)
        {
            parse_info pi(m_path, line, lineNumber);
            const auto m = pi.toString();
            logexception_msg(ioe, m);
            // cleanup file
            stepHdrFile.close();
            throw ioe;
        }

        return true;
	}

	std::unique_ptr<Odb::Lib::Protobuf::StepHdrFile> StepHdrFile::to_protobuf() const
	{
		auto message = std::make_unique<Odb::Lib::Protobuf::StepHdrFile>();
		message->set_xdatum(xDatum);
		message->set_ydatum(yDatum);
		message->set_id(id);
		message->set_xorigin(xOrigin);
		message->set_yorigin(yOrigin);
		message->set_topactive(topActive);
		message->set_bottomactive(bottomActive);
		message->set_leftactive(leftActive);
		message->set_rightactive(rightActive);
		message->set_affectingbom(affectingBom);
		message->set_affectingbomchanged(affectingBomChanged);		

		for (const auto& stepRepeatRecord : m_stepRepeatRecords)
		{
			auto stepRepeatRecordPtr = message->add_steprepeatrecords();
			stepRepeatRecordPtr->CopyFrom(*stepRepeatRecord->to_protobuf());
		}

        for (const auto& kvOnlineValue : m_onlineValues)
        {
            (*message->mutable_onlinevalues())[kvOnlineValue.first] = kvOnlineValue.second;
        }

		return message;
	}

	void StepHdrFile::from_protobuf(const Odb::Lib::Protobuf::StepHdrFile& message)
	{
		xDatum = message.xdatum();
		yDatum = message.ydatum();
		id = message.id();
		xOrigin = message.xorigin();
		yOrigin = message.yorigin();
		topActive = message.topactive();
		bottomActive = message.bottomactive();
		leftActive = message.leftactive();
		rightActive = message.rightactive();
		affectingBom = message.affectingbom();
		affectingBomChanged = message.affectingbomchanged();		
		
		for (const auto& stepRepeatRecord : message.steprepeatrecords())
		{
			auto stepRepeatRecordPtr = std::make_unique<StepRepeatRecord>();
			stepRepeatRecordPtr->from_protobuf(stepRepeatRecord);
			m_stepRepeatRecords.push_back(std::move(stepRepeatRecordPtr));
		}

        for (const auto& kvOnlineValue : message.onlinevalues())
        {
            m_onlineValues[kvOnlineValue.first] = kvOnlineValue.second;
        }
	}

	std::unique_ptr<Odb::Lib::Protobuf::StepHdrFile::StepRepeatRecord> StepHdrFile::StepRepeatRecord::to_protobuf() const
	{
		auto message = std::make_unique<Odb::Lib::Protobuf::StepHdrFile::StepRepeatRecord>();
		message->set_name(name);
		message->set_x(x);
		message->set_y(y);
		message->set_dx(dx);
		message->set_dy(dy);
		message->set_nx(nx);
		message->set_ny(ny);
		message->set_angle(angle);
        message->set_flip(flip);
        message->set_mirror(mirror);
		return message;		
	}

	void StepHdrFile::StepRepeatRecord::from_protobuf(const Odb::Lib::Protobuf::StepHdrFile::StepRepeatRecord& message)
	{
		name = message.name();
		x = message.x();
		y = message.y();
		dx = message.dx();
		dy = message.dy();
		nx = message.nx();
		ny = message.ny();
        flip = message.flip();
        mirror = message.mirror();
		angle = message.angle();
	}
}