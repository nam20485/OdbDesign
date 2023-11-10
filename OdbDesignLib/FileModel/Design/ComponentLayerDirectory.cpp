#include "ComponentLayerDirectory.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include "ArchiveExtractor.h"
#include "../parse_error.h"
#include <Logger.h>
#include <exception>
#include <str_trim.h>
#include "../invalid_odb_error.h"
#include <climits>

using namespace std::filesystem;


namespace Odb::Lib::FileModel::Design
{
	ComponentLayerDirectory::ComponentLayerDirectory(std::filesystem::path path, BoardSide side)
		: LayerDirectory(path), m_id(0)
		, m_side(side)
	{
	}

	ComponentLayerDirectory::~ComponentLayerDirectory()
	{
		m_attributeNames.clear();
		m_attributeTextValues.clear();
		m_componentRecords.clear();
		m_componentRecordsByName.clear();
	}

	std::string ComponentLayerDirectory::GetUnits() const
	{
		return m_units;
	}

	BoardSide ComponentLayerDirectory::GetSide() const
	{
		return m_side;
	}

	const ComponentLayerDirectory::ComponentRecord::Vector& ComponentLayerDirectory::GetComponentRecords() const
	{
		return m_componentRecords;
	}

	const ComponentLayerDirectory::ComponentRecord::StringMap& ComponentLayerDirectory::GetComponentRecordsByName() const
	{
		return m_componentRecordsByName;
	}

	const std::vector<std::string>& ComponentLayerDirectory::GetAttributeNames() const
	{
		return m_attributeNames;
	}

	const std::vector<std::string>& ComponentLayerDirectory::GetAttributeTextValues() const
	{
		return m_attributeTextValues;
	}

	ComponentLayerDirectory::ComponentRecord::~ComponentRecord()
	{
		m_toeprintRecords.clear();
		m_propertyRecords.clear();
	}

	bool ComponentLayerDirectory::Parse()
	{
		std::ifstream componentsFile;
		int lineNumber = 0;
		std::string line;

		try
		{
			if (!LayerDirectory::Parse()) return false;

			loginfo("checking for extraction...");

			std::filesystem::path componentsFilePath;
			for (const std::string componentsFilename : COMPONENTS_FILENAMES)
			{
				loginfo("trying components file: [" + componentsFilename + "]...");

				componentsFilePath = Utils::ArchiveExtractor::getUncompressedFilePath(m_path, componentsFilename);
				if (exists(componentsFilePath) && is_regular_file(componentsFilePath))
				{			
					loginfo("found components file: [" + componentsFilePath.string() + "]");
					break;
				}
			}			

			loginfo("any extraction complete, parsing data...");

			if (!std::filesystem::exists(componentsFilePath))
			{
				auto message = "components file does not exist: [" + m_path.string() + "]";				
				throw invalid_odb_error(message.c_str());
			}
			else if (!std::filesystem::is_regular_file(componentsFilePath))
			{
				auto message = "components is not a file: [" + m_path.string() + "]";
				throw invalid_odb_error(message.c_str());
			}
			
			componentsFile.open(componentsFilePath.string(), std::ios::in);
			if (!componentsFile.is_open())
			{
				auto message = "unable to open components file: [" + m_path.string() + "]";
				throw invalid_odb_error(message.c_str());
			}

			std::shared_ptr<ComponentRecord> pCurrentComponentRecord;
			
			while (std::getline(componentsFile, line))
			{
				lineNumber++;

				// trim whitespace from beginning and end of line
				Utils::str_trim(line);
				if (!line.empty())
				{
					std::stringstream lineStream(line);
					//char firstChar = line[0];			

					if (line.find(COMMENT_TOKEN) == 0)
					{
						// comment line
					}
					else if (line.find(UNITS_TOKEN) == 0)
					{
						// units line
						std::string token;
						if (!std::getline(lineStream, token, '='))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}
						else if (!std::getline(lineStream, token, '='))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						m_units = token;
					}
					else if (line.find(ID_TOKEN) == 0)
					{
						std::string token;
						if (!std::getline(lineStream, token, '='))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}
						else if (!std::getline(lineStream, token, '='))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}
						m_id = std::stoul(token);
					}
					else if (line.find(ATTRIBUTE_NAME_TOKEN) == 0)
					{
						// component attribute name line	
						std::string token;
						// TODO: continue on failing line parse, to make a less strict/more robust parser (make a flag: enum ParseStrictness { strict, lax })
						if (!std::getline(lineStream, token, ' '))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}
						else if (!std::getline(lineStream, token, ' '))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}
						m_attributeNames.push_back(token);
					}
					else if (line.find(ATTRIBUTE_VALUE_TOKEN) == 0)
					{
						// component attribute text string values	
						std::string token;
						if (!std::getline(lineStream, token, ' '))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}
						else if (!std::getline(lineStream, token, ' '))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}
						m_attributeTextValues.push_back(token);
					}
					else if (line.find(ComponentRecord::RECORD_TOKEN) == 0)
					{
						// component record line
						std::string token;

						lineStream >> token;
						if (token != ComponentRecord::RECORD_TOKEN)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						// do we have a current component record?
						if (pCurrentComponentRecord != nullptr)
						{
							// finish it up and add it to the list
							m_componentRecords.push_back(pCurrentComponentRecord);
							pCurrentComponentRecord.reset();
						}

						// create a new current component record
						pCurrentComponentRecord = std::make_shared<ComponentRecord>();

						lineStream >> pCurrentComponentRecord->pkgRef;
						lineStream >> pCurrentComponentRecord->locationX;
						lineStream >> pCurrentComponentRecord->locationY;
						lineStream >> pCurrentComponentRecord->rotation;

						char mirror;
						lineStream >> mirror;
						pCurrentComponentRecord->mirror = (mirror == 'M');

						lineStream >> pCurrentComponentRecord->compName;
						lineStream >> pCurrentComponentRecord->partName;
						lineStream >> pCurrentComponentRecord->attributes;

						if (m_componentRecords.size() > UINT_MAX)
						{
							throw_parse_error(m_path, line, token, lineNumber);							
						}
						
						pCurrentComponentRecord->index = static_cast<unsigned int>(m_componentRecords.size());

						// TODO: parse attributes and id string
						//std::string strId;
						//lineStream >> strId;
						//std::stringstream idStream(strId);
						//if (!std::getline(idStream, token, '=')) return false;
						//else if (!std::getline(idStream, token, '=')) return false;
						//pCurrentComponentRecord->id = std::stoul(token);

					}
					else if (line.find(ComponentRecord::PropertyRecord::RECORD_TOKEN) == 0)
					{
						// component property record line
						std::string token;
						if (!(lineStream >> token))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (token != ComponentRecord::PropertyRecord::RECORD_TOKEN)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						auto pPropertyRecord = std::make_shared<ComponentRecord::PropertyRecord>();

						if (!(lineStream >> pPropertyRecord->name))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> token))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!token.empty())
						{
							// handle case where the beginning of the value, after the opening single-quote, is whitespace...
							if (token == "'")
							{
								// eat the single-quote and get the next token, i.e. the actual value (w/ space in front)
								if (!(lineStream >> token))
								{
									throw_parse_error(m_path, line, token, lineNumber);
								}
							}

							if (token.size() > 0 && token[0] == '\'')
							{
								// remove leading quote							
								token.erase(0, 1);
							}

							if (token.size() > 0 && token[token.size() - 1] == '\'')
							{
								// remove trailing quote
								token.erase(token.size() - 1);
							}
							
							Utils::str_trim(token);
						}	

						pPropertyRecord->value = token;

						float f;
						while (lineStream >> f)
						{
							pPropertyRecord->floatValues.push_back(f);
						}

						pCurrentComponentRecord->m_propertyRecords.push_back(pPropertyRecord);
					}
					else if (line.find(ComponentRecord::ToeprintRecord::RECORD_TOKEN) == 0)
					{
						// component toeprint record line
						std::string token;
						lineStream >> token;
						if (token != ComponentRecord::ToeprintRecord::RECORD_TOKEN)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						auto pToeprintRecord = std::make_shared<ComponentRecord::ToeprintRecord>();
						lineStream >> pToeprintRecord->pinNumber;
						lineStream >> pToeprintRecord->locationX;
						lineStream >> pToeprintRecord->locationY;
						lineStream >> pToeprintRecord->rotation;

						char mirror;
						lineStream >> mirror;
						pToeprintRecord->mirror = (mirror == 'M' || mirror == 'm');

						lineStream >> pToeprintRecord->netNumber;
						if (pToeprintRecord->netNumber == (unsigned int)-1)
						{
							// netNumber == -1
							parse_info pi(m_path, line, lineNumber);
							logwarn(pi.toString("Component Toeprint record with netNumber = -1"));

							if (!m_allowToepintNetNumbersOfNegative1)
							{
								throw_parse_error(m_path, line, token, lineNumber);
							}							
						}

						lineStream >> pToeprintRecord->subnetNumber;
						if (pToeprintRecord->subnetNumber == (unsigned int)-1)
						{
							// subnetNumber == -1
							parse_info pi(m_path, line, lineNumber);
							logwarn(pi.toString("Component Toeprint record with subnetNumber = -1"));

							if (!m_allowToepintNetNumbersOfNegative1)
							{
								throw_parse_error(m_path, line, token, lineNumber);
							}
						}

						lineStream >> pToeprintRecord->name;

						pCurrentComponentRecord->m_toeprintRecords.push_back(pToeprintRecord);
					}
				}
			}

			// do we have a current component record? (finish up the last record in the file- i.e. ran out of file)
			if (pCurrentComponentRecord != nullptr)
			{
				// finish it up and add it to the list
				m_componentRecords.push_back(pCurrentComponentRecord);
				pCurrentComponentRecord.reset();
			}

			componentsFile.close();			
		}
		catch (parse_error& pe)
		{
			auto m = pe.toString("Parse Error:");
			logerror(m);
			componentsFile.close();			
			throw pe;
		}
		catch (std::exception& e)
		{
			parse_info pi(m_path, line, lineNumber);
			const auto m = pi.toString();
			logexception_msg(e, m);			
			componentsFile.close();
			throw e;
		}

		return true;
	}
}