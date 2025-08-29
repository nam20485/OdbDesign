//
// Created by Jandody on 08/26/2025.
//

#include "ToolsFile.h"
#include <fstream>
#include "../parse_error.h"
#include "../invalid_odb_error.h"
#include "Logger.h"
#include "str_utils.h"
#include "../../Constants.h"
#include <ArchiveExtractor.h>

namespace Odb::Lib::FileModel::Design
{
	ToolsFile::ToolsFile()
		: m_path("")
		, m_directory("")
		, m_units("")
		, m_thickness(0)
		, m_user_params("")
	{
	}

	ToolsFile::~ToolsFile()
	{
		m_toolsByNum.clear();
	}

	std::string ToolsFile::GetUnits() const
	{
		return m_units;
	}

	double ToolsFile::GetThickness() const
	{
		return m_thickness;
	}

	std::string ToolsFile::GetUserParams() const
	{
		return m_user_params;
	}

	const ToolsFile::ToolsMap& ToolsFile::GetTools() const
	{
		return m_toolsByNum;
	}

	bool Lib::FileModel::Design::ToolsFile::Parse(std::filesystem::path directory)
	{
		std::ifstream toolsFile;
		int lineNumber = 0;
		std::string line;

		try
		{
			m_directory = directory;

			loginfo("checking for extraction...");

			std::filesystem::path toolsFilePath;
			for (const std::string toolsFilename : TOOLS_FILENAMES)
			{
				loginfo("trying attrlist file: [" + toolsFilename + "]...");

				toolsFilePath = Utils::ArchiveExtractor::getUncompressedFilePath(m_directory, toolsFilename);
				if (exists(toolsFilePath) && is_regular_file(toolsFilePath))
				{
					loginfo("found attrlist file: [" + toolsFilePath.string() + "]");
					break;
				}
			}

			m_path = toolsFilePath;

			loginfo("any extraction complete, parsing data...");

			if (!std::filesystem::exists(m_path))
			{
				auto message = "attrlist file does not exist: [" + m_directory.string() + "]";
				logwarn(message);
				return true;
				//throw invalid_odb_error(message.c_str());
			}
			else if (!std::filesystem::is_regular_file(m_path))
			{
				auto message = "attrlist is not a file: [" + m_path.string() + "]";
				throw invalid_odb_error(message.c_str());
			}

			toolsFile.open(m_path.string(), std::ios::in);
			if (!toolsFile.is_open())
			{
				auto message = "unable to open attrlist file: [" + m_path.string() + "]";
				throw invalid_odb_error(message.c_str());
			}

			std::shared_ptr<ToolsRecord> pCurrentToolsRecord;
			bool openBraceFound = false;

			while (std::getline(toolsFile, line))
			{
				lineNumber++;

				// trim whitespace from beginning and end of line
				Utils::str_trim(line);
				if (!line.empty())
				{
					std::stringstream lineStream(line);
					//char firstChar = line[0];			

					if (line.find(Constants::COMMENT_TOKEN) == 0)
					{
						// comment line
					}
					else if (line.find(ToolsFile::UNITS_TOKEN) == 0)
					{
						// units line
						std::string token;
						if (!std::getline(lineStream, token, '='))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						m_units = token;
					}
					else if (line.find(ToolsFile::THICKNESS_TOKEN) == 0)
					{
						// thickness line
						std::string value;
						if (!std::getline(lineStream, value, '='))
						{
							throw_parse_error(m_path, line, value, lineNumber);
						}

						if (std::getline(lineStream, value))
						{
							Utils::str_trim(value);
							m_thickness = std::stod(value);
						}
						else
						{
							// Obsolete, Default=0
							m_thickness = 0;
						}

					}
					else if (line.find(ToolsFile::USER_PARAMS_TOKEN) == 0)
					{
						// user_params line
						std::string value;
						if (!std::getline(lineStream, value, '='))
						{
							throw_parse_error(m_path, line, value, lineNumber);
						}

						if (std::getline(lineStream, value))
						{
							Utils::str_trim(value);
							m_user_params = value;
						}
					}
					else if (line.find(ToolsRecord::RECORD_TOKEN) == 0)
					{
						std::string token;
						if (!(lineStream >> token))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (token != ToolsRecord::RECORD_TOKEN || pCurrentToolsRecord != nullptr)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						// open a new TOOL array record
						pCurrentToolsRecord = std::make_shared<ToolsRecord>();

						if (lineStream >> token)
						{
							// open brace is on same line as tool record open token
							if (token == Constants::ARRAY_RECORD_OPEN_TOKEN)
							{
								openBraceFound = true;
							}
						}
					}
					else if (line.find(Constants::ARRAY_RECORD_OPEN_TOKEN) == 0)
					{
						// no current opening of a tool array record found yet
						if (pCurrentToolsRecord == nullptr)
						{
							throw_parse_error(m_path, line, "", lineNumber);
						}

						// found another open brace while still parsing after the previous record's open brace
						if (openBraceFound)
						{
							throw_parse_error(m_path, line, "", lineNumber);
						}

						openBraceFound = true;
					}
					else if (line.find(Constants::ARRAY_RECORD_CLOSE_TOKEN) == 0)
					{
						if (pCurrentToolsRecord != nullptr && openBraceFound && pCurrentToolsRecord->toolNum != 0)
						{
							m_toolsByNum[pCurrentToolsRecord->toolNum] = pCurrentToolsRecord;
							pCurrentToolsRecord.reset();
							openBraceFound = false;
						}
						else
						{
							// found a close brace but aren't currently parsing a tools map record
							throw_parse_error(m_path, line, "", lineNumber);
						}
					}
					else
					{
						std::string attribute;
						std::string value;

						if (!std::getline(lineStream, attribute, '='))
						{
							throw_parse_error(m_path, line, "", lineNumber);
						}

						if (std::getline(lineStream, value))
						{
							Utils::str_trim(attribute);
							Utils::str_trim(value);
							
							if (pCurrentToolsRecord != nullptr && openBraceFound)
							{
								if (attribute == ToolsRecord::NUM_KEY || attribute == "num")
								{
									pCurrentToolsRecord->toolNum = std::stoi(value);
								}
								else if (attribute == ToolsRecord::TYPE_KEY || attribute == "type")
								{
									if (ToolsRecord::typeMap.contains(value))
									{
										pCurrentToolsRecord->type = ToolsRecord::typeMap.getValue(value);
									}
									else
									{
										throw_parse_error(m_path, line, attribute, lineNumber);
									}
								}
								else if (attribute == ToolsRecord::TYPE2_KEY || attribute == "type2")
								{
									if (ToolsRecord::type2Map.contains(value))
									{
										auto type = pCurrentToolsRecord->type;
										auto type2 = ToolsRecord::type2Map.getValue(value);
										
										if (type2 != ToolsRecord::Type2::Standard)
										{
											bool invalid = false;
											switch (type2)
											{
											case ToolsRecord::Type2::PressFit:
												invalid = (type != ToolsRecord::Type::Plated);
												break;

											case ToolsRecord::Type2::Photo:
											case ToolsRecord::Type2::Laser:
												invalid = (type != ToolsRecord::Type::Via);
												break;
											}
											if (invalid)
											{
												throw_parse_error(m_path, line, attribute, lineNumber);
											}
										}
										pCurrentToolsRecord->type2 = type2;
									}
									else
									{
										pCurrentToolsRecord->type2 = ToolsRecord::Type2::Standard;
									}
								}
								else if (attribute == ToolsRecord::MIN_TOL_KEY || attribute == "min_tol")
								{
									pCurrentToolsRecord->minTOL = std::stod(value);
								}
								else if (attribute == ToolsRecord::MAX_TOL_KEY || attribute == "max_tol")
								{
									pCurrentToolsRecord->maxTOL = std::stod(value);
								}
								else if (attribute == ToolsRecord::BIT_KEY || attribute == "bit")
								{
									pCurrentToolsRecord->driilBit = value;
								}
								else if (attribute == ToolsRecord::FINISH_SIZE_KEY || attribute == "finish_size")
								{
									pCurrentToolsRecord->FinishSize = std::stod(value);
								}
								else if (attribute == ToolsRecord::DRILL_SIZE_KEY || attribute == "drill_size")
								{
									pCurrentToolsRecord->DrillSize = std::stod(value);
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

					}
				}
			}

			toolsFile.close();
		}
		catch (parse_error& pe)
		{
			auto m = pe.toString("Parse Error:");
			logerror(m);
			toolsFile.close();
			throw pe;
		}
		catch (std::exception& e)
		{
			parse_info pi(m_path, line, lineNumber);
			const auto m = pi.toString();
			logexception_msg(e, m);
			toolsFile.close();
			throw e;
		}

		return true;
	}

	bool ToolsFile::Save(std::ostream& os)
	{
		os << Constants::UNITS_TOKEN << " = " << m_units << std::endl;
		os << ToolsFile::THICKNESS_TOKEN << " = " << std::to_string(m_thickness) << std::endl;
		os << ToolsFile::USER_PARAMS_TOKEN << " = " << m_user_params << std::endl;

		for (const auto& [toolNum, tool_info] : m_toolsByNum)
		{
			os << ToolsRecord::RECORD_TOKEN << " " << Constants::ARRAY_RECORD_OPEN_TOKEN << std::endl;

			os << '\t' << ToolsRecord::NUM_KEY << "=" << toolNum << std::endl;
			os << '\t' << ToolsRecord::TYPE_KEY << "=" << ToolsRecord::typeMap.getValue(tool_info->type) << std::endl;
			os << '\t' << ToolsRecord::TYPE2_KEY << "=" << ToolsRecord::type2Map.getValue(tool_info->type2) << std::endl;
			os << '\t' << ToolsRecord::MIN_TOL_KEY << "=" << tool_info->minTOL << std::endl;
			os << '\t' << ToolsRecord::MAX_TOL_KEY << "=" << tool_info->maxTOL << std::endl;
			os << '\t' << ToolsRecord::BIT_KEY << "=" << tool_info->driilBit << std::endl;
			os << '\t' << ToolsRecord::FINISH_SIZE_KEY << "=" << tool_info->FinishSize << std::endl;
			os << '\t' << ToolsRecord::DRILL_SIZE_KEY << "=" << tool_info->DrillSize << std::endl;

			os << Constants::ARRAY_RECORD_CLOSE_TOKEN << std::endl;
			os << std::endl;
		}

		return true;
	}

	std::unique_ptr<Odb::Lib::Protobuf::ToolsFile::ToolsRecord>Odb::Lib::FileModel::Design::ToolsFile::ToolsRecord::to_protobuf() const
	{
		std::unique_ptr<Odb::Lib::Protobuf::ToolsFile::ToolsRecord> pToolsRecordMessage(new Odb::Lib::Protobuf::ToolsFile::ToolsRecord);
		pToolsRecordMessage->set_tool_num(toolNum);
		pToolsRecordMessage->set_type(static_cast<Odb::Lib::Protobuf::ToolsFile::ToolsRecord::Type>(type));
		pToolsRecordMessage->set_type2(static_cast<Odb::Lib::Protobuf::ToolsFile::ToolsRecord::Type2>(type2));
		pToolsRecordMessage->set_min_tol(minTOL);
		pToolsRecordMessage->set_max_tol(maxTOL);
		pToolsRecordMessage->set_drill_bit(driilBit);
		pToolsRecordMessage->set_finish_size(FinishSize);
		pToolsRecordMessage->set_drill_size(DrillSize);
		
		return pToolsRecordMessage;
	}

	void Odb::Lib::FileModel::Design::ToolsFile::ToolsRecord::from_protobuf(const Odb::Lib::Protobuf::ToolsFile::ToolsRecord& message)
	{
		toolNum = message.tool_num();
		type = static_cast<Type>(message.type());
		type2 = static_cast<Type2>(message.type2());
		minTOL = message.min_tol();
		maxTOL = message.max_tol();
		driilBit = message.drill_bit();
		FinishSize = message.finish_size();
		DrillSize = message.drill_size();
	}

	std::unique_ptr<Odb::Lib::Protobuf::ToolsFile> Lib::FileModel::Design::ToolsFile::to_protobuf() const
	{
		auto message = std::make_unique<Odb::Lib::Protobuf::ToolsFile>();

		message->set_directory(m_directory.string());
		message->set_path(m_path.string());
		message->set_units(m_units);

		message->set_thickness(m_thickness);
		message->set_user_params(m_user_params);

		auto* tools_map = message->mutable_tools();
		for (const auto& [toolNum, toolsRecord] : m_toolsByNum)
		{
			auto& recordMsg = (*tools_map)[toolNum];
			recordMsg.CopyFrom(*toolsRecord->to_protobuf());
		}

		return message;
	}

	void Lib::FileModel::Design::ToolsFile::from_protobuf(const Odb::Lib::Protobuf::ToolsFile& message)
	{
		m_directory = message.directory();
		m_path = message.path();

		m_units = message.units();
		m_thickness = message.thickness();
		m_user_params = message.user_params();

		for (const auto& [toolNum, toolsRecord] : message.tools())
		{
			auto ptoolsRecord = std::make_shared<ToolsRecord>();
			ptoolsRecord->from_protobuf(toolsRecord);
			m_toolsByNum.emplace(toolNum, ptoolsRecord);
		}
	}

} // namespace Odb::Lib::FileModel::Design