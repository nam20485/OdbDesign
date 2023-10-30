#include "ComponentLayerDirectory.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include "ArchiveExtractor.h"
#include "../parse_error.h"
#include <Logger.h>


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
		try
		{
			if (!LayerDirectory::Parse()) return false;

			auto componentsFilePath = Utils::ArchiveExtractor::getUncompressedFilePath(m_path, COMPONENTS_FILENAME);

			if (!std::filesystem::exists(componentsFilePath))
			{
				throw_parse_error(m_path, "", "", -1);
			}
			else if (!std::filesystem::is_regular_file(componentsFilePath))
			{
				throw_parse_error(m_path, "", "", -1);
			}

			std::ifstream componentsFile;
			componentsFile.open(componentsFilePath.string(), std::ios::in);
			if (!componentsFile.is_open())
			{
				throw_parse_error(m_path, "", "", -1);
			}

			std::shared_ptr<ComponentRecord> pCurrentComponentRecord;

			int lineNumber = 0;
			std::string line;
			while (std::getline(componentsFile, line))
			{
				lineNumber++;
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
						lineStream >> token;
						if (token != ComponentRecord::PropertyRecord::RECORD_TOKEN)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						auto pPropertyRecord = std::make_shared<ComponentRecord::PropertyRecord>();
						lineStream >> pPropertyRecord->name;

						lineStream >> pPropertyRecord->value;
						// remove leading quote
						pPropertyRecord->value.erase(0, 1);
						// remove trailing quote
						pPropertyRecord->value.erase(pPropertyRecord->value.size() - 1);

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
						pToeprintRecord->mirror = (mirror == 'M');

						lineStream >> pToeprintRecord->netNumber;
						lineStream >> pToeprintRecord->subnetNumber;
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
			auto m = pe.buildMessage("Parse Error:");
			logerror(m);
			//return false;
			throw pe;
		}

		return true;
	}
}