#include "ComponentsFile.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include "ArchiveExtractor.h"
#include "../parse_error.h"
#include <Logger.h>
#include <exception>
#include "str_utils.h"
#include "../invalid_odb_error.h"
#include <climits>

using namespace std::filesystem;


namespace Odb::Lib::FileModel::Design
{
	ComponentsFile::ComponentsFile()
		: m_id((unsigned int)-1)
		, m_side(BoardSide::BsNone)
	{
	}

	ComponentsFile::~ComponentsFile()
	{
		m_attributeNames.clear();
		m_attributeTextValues.clear();
		m_componentRecords.clear();
		m_componentRecordsByName.clear();
		m_bomDescriptionRecordsByCpn.clear();
	}

	std::string ComponentsFile::GetUnits() const
	{
		return m_units;
	}

	BoardSide ComponentsFile::GetSide() const
	{
		return m_side;
	}

	std::filesystem::path ComponentsFile::GetPath()
	{
		return m_path;
	}

	std::filesystem::path ComponentsFile::GetDirectory()
	{
		return m_directory;
	}

	std::string ComponentsFile::GetLayerName() const
	{
		return m_layerName;
	}

	const ComponentsFile::ComponentRecord::Vector& ComponentsFile::GetComponentRecords() const
	{
		return m_componentRecords;
	}

	const ComponentsFile::ComponentRecord::StringMap& ComponentsFile::GetComponentRecordsByName() const
	{
		return m_componentRecordsByName;
	}

	const std::vector<std::string>& ComponentsFile::GetAttributeNames() const
	{
		return m_attributeNames;
	}

	const std::vector<std::string>& ComponentsFile::GetAttributeTextValues() const
	{
		return m_attributeTextValues;
	}

	const ComponentsFile::BomDescriptionRecord::StringMap& ComponentsFile::GetBomDescriptionRecordsByCpn() const
	{
		return m_bomDescriptionRecordsByCpn;
	}

	std::unique_ptr<Odb::Lib::Protobuf::ComponentsFile> ComponentsFile::to_protobuf() const
	{
		std::unique_ptr<Odb::Lib::Protobuf::ComponentsFile> pComponentsFileMessage(new Odb::Lib::Protobuf::ComponentsFile);
		pComponentsFileMessage->set_units(m_units);
		pComponentsFileMessage->set_id(m_id);
		pComponentsFileMessage->set_side(static_cast<Odb::Lib::Protobuf::BoardSide>(m_side));
		pComponentsFileMessage->set_layername(m_layerName);
		pComponentsFileMessage->set_path(m_path.string());
		pComponentsFileMessage->set_directory(m_directory.string());

		for (const auto& attributeName : m_attributeNames)
		{
			pComponentsFileMessage->add_attributenames(attributeName);
		}

		for (const auto& attributeTextValue : m_attributeTextValues)
		{
			pComponentsFileMessage->add_attributetextvalues(attributeTextValue);
		}

		for (const auto& pComponentRecord : m_componentRecords)
		{
			auto pComponentRecordMessage = pComponentRecord->to_protobuf();
			pComponentsFileMessage->add_componentrecords()->CopyFrom(*pComponentRecordMessage);
		}

		for (const auto& kvBomDescriptionRecord : m_bomDescriptionRecordsByCpn)
		{
			auto pBomDescriptionRecordMessage = kvBomDescriptionRecord.second->to_protobuf();
			(*pComponentsFileMessage->mutable_bomdescriptionrecordsbycpn())[kvBomDescriptionRecord.first] = *pBomDescriptionRecordMessage;
		}

		return pComponentsFileMessage;		
	}

	void ComponentsFile::from_protobuf(const Odb::Lib::Protobuf::ComponentsFile& message)
	{
		m_units = message.units();
		m_id = message.id();
		m_side = static_cast<BoardSide>(message.side());
		m_layerName = message.layername();
		m_path = message.path();
		m_directory = message.directory();

		for (const auto& attributeName : message.attributenames())
		{
			m_attributeNames.push_back(attributeName);
		}

		for (const auto& attributeTextValue : message.attributetextvalues())
		{
			m_attributeTextValues.push_back(attributeTextValue);
		}

		for (const auto& componentRecordMessage : message.componentrecords())
		{
			auto pComponentRecord = std::make_shared<ComponentRecord>();
			pComponentRecord->from_protobuf(componentRecordMessage);
			m_componentRecords.push_back(pComponentRecord);
		}

		for (const auto& kvBomDescriptionRecord : message.bomdescriptionrecordsbycpn())
		{
			auto pBomDescriptionRecord = std::make_shared<BomDescriptionRecord>();
			pBomDescriptionRecord->from_protobuf(kvBomDescriptionRecord.second);
			m_bomDescriptionRecordsByCpn[kvBomDescriptionRecord.first] = pBomDescriptionRecord;
		}
	}

	bool ComponentsFile::Save(std::ostream& os)
	{
		return true;
	}

	std::unique_ptr<Odb::Lib::Protobuf::ComponentsFile::BomDescriptionRecord> ComponentsFile::BomDescriptionRecord::to_protobuf() const
	{
		std::unique_ptr<Odb::Lib::Protobuf::ComponentsFile::BomDescriptionRecord> pBomDescriptionRecordMessage(new Odb::Lib::Protobuf::ComponentsFile::BomDescriptionRecord);
		pBomDescriptionRecordMessage->set_cpn(cpn);
		pBomDescriptionRecordMessage->set_pkg(pkg);
		pBomDescriptionRecordMessage->set_ipn(ipn);
		for (const auto& description : descriptions)
		{			
			pBomDescriptionRecordMessage->add_descriptions()->assign(description);
		}
		pBomDescriptionRecordMessage->set_vpl_vnd(vpl_vnd);
		pBomDescriptionRecordMessage->set_vpl_mpn(vpl_mpn);
		pBomDescriptionRecordMessage->set_vnd(vnd);
		pBomDescriptionRecordMessage->set_mpn(mpn);
		return pBomDescriptionRecordMessage;
	}

	void ComponentsFile::BomDescriptionRecord::from_protobuf(const Odb::Lib::Protobuf::ComponentsFile::BomDescriptionRecord& message)
	{
		cpn = message.cpn();
		pkg = message.pkg();
		ipn = message.ipn();
		for (const auto& description : message.descriptions())
		{
			descriptions.push_back(description);
		}
		vpl_vnd = message.vpl_vnd();
		vpl_mpn = message.vpl_mpn();
		vnd = message.vnd();
		mpn = message.mpn();
	}

	ComponentsFile::ComponentRecord::~ComponentRecord()
	{
		m_toeprintRecords.clear();
		m_propertyRecords.clear();
	}

	std::unique_ptr<Odb::Lib::Protobuf::ComponentsFile::ComponentRecord> ComponentsFile::ComponentRecord::to_protobuf() const
	{
		std::unique_ptr<Odb::Lib::Protobuf::ComponentsFile::ComponentRecord> pComponentRecordMessage(new Odb::Lib::Protobuf::ComponentsFile::ComponentRecord);
		pComponentRecordMessage->set_pkgref(pkgRef);
		pComponentRecordMessage->set_locationx(locationX);
		pComponentRecordMessage->set_locationy(locationY);
		pComponentRecordMessage->set_rotation(rotation);
		pComponentRecordMessage->set_mirror(mirror);
		pComponentRecordMessage->set_compname(compName);
		pComponentRecordMessage->set_partname(partName);
		//pComponentRecordMessage->set_attributes(attributes);
		pComponentRecordMessage->set_index(index);

		for (const auto& pPropertyRecord : m_propertyRecords)
		{
			auto pPropertyRecordMessage = pPropertyRecord->to_protobuf();
			pComponentRecordMessage->add_propertyrecords()->CopyFrom(*pPropertyRecordMessage);
		}

		for (const auto& pToeprintRecord : m_toeprintRecords)
		{
			auto pToeprintRecordMessage = pToeprintRecord->to_protobuf();
			pComponentRecordMessage->add_toeprintrecords()->CopyFrom(*pToeprintRecordMessage);
		}

		for (const auto& kvAttributeAssignment : m_attributeLookupTable)
		{
			(*pComponentRecordMessage->mutable_attributelookuptable())[kvAttributeAssignment.first] = kvAttributeAssignment.second;
		}

		return pComponentRecordMessage;
	}

	void ComponentsFile::ComponentRecord::from_protobuf(const Odb::Lib::Protobuf::ComponentsFile::ComponentRecord& message)
	{
		pkgRef = message.pkgref();
		locationX = message.locationx();
		locationY = message.locationy();
		rotation = message.rotation();
		mirror = message.mirror();
		compName = message.compname();
		partName = message.partname();
		//attributes = message.attributes();
		index = message.index();

		for (const auto& propertyRecordMessage : message.propertyrecords())
		{
			auto pPropertyRecord = std::make_shared<PropertyRecord>();
			pPropertyRecord->from_protobuf(propertyRecordMessage);
			m_propertyRecords.push_back(pPropertyRecord);
		}

		for (const auto& toeprintRecordMessage : message.toeprintrecords())
		{
			auto pToeprintRecord = std::make_shared<ToeprintRecord>();
			pToeprintRecord->from_protobuf(toeprintRecordMessage);
			m_toeprintRecords.push_back(pToeprintRecord);
		}

		for (const auto& kvAttributeAssignment : message.attributelookuptable())
		{
			m_attributeLookupTable[kvAttributeAssignment.first] = kvAttributeAssignment.second;
		}
	}

	std::unique_ptr<Odb::Lib::Protobuf::ComponentsFile::ComponentRecord::ToeprintRecord> ComponentsFile::ComponentRecord::ToeprintRecord::to_protobuf() const
	{
		std::unique_ptr<Odb::Lib::Protobuf::ComponentsFile::ComponentRecord::ToeprintRecord> pToeprintRecordMessage(new Odb::Lib::Protobuf::ComponentsFile::ComponentRecord::ToeprintRecord);
		pToeprintRecordMessage->set_pinnumber(pinNumber);
		pToeprintRecordMessage->set_locationx(locationX);
		pToeprintRecordMessage->set_locationy(locationY);
		pToeprintRecordMessage->set_rotation(rotation);
		pToeprintRecordMessage->set_mirror(mirror);
		pToeprintRecordMessage->set_netnumber(netNumber);
		pToeprintRecordMessage->set_subnetnumber(subnetNumber);
		pToeprintRecordMessage->set_name(name);
		return pToeprintRecordMessage;		
	}

	void ComponentsFile::ComponentRecord::ToeprintRecord::from_protobuf(const Odb::Lib::Protobuf::ComponentsFile::ComponentRecord::ToeprintRecord& message)
	{
		pinNumber = message.pinnumber();
		locationX = message.locationx();
		locationY = message.locationy();
		rotation = message.rotation();
		mirror = message.mirror();
		netNumber = message.netnumber();
		subnetNumber = message.subnetnumber();
		name = message.name();
	}

	bool ComponentsFile::Parse(std::filesystem::path directory)
	{
		std::ifstream componentsFile;
		int lineNumber = 0;
		std::string line;

		try
		{
			m_directory = directory;

			m_layerName = m_directory.filename().string();
			if (m_layerName == TOP_COMPONENTS_LAYER_NAME ||
				m_layerName == BOTTOM_COMPONENTS_LAYER_NAME)
			{
				m_side = m_layerName == TOP_COMPONENTS_LAYER_NAME ?
					BoardSide::Top :
					BoardSide::Bottom;
			}
			else
			{
				// not a components layer
				return true;
			}

			loginfo("checking for extraction...");

			std::filesystem::path componentsFilePath;
			for (const std::string componentsFilename : COMPONENTS_FILENAMES)
			{
				loginfo("trying components file: [" + componentsFilename + "]...");

				componentsFilePath = Utils::ArchiveExtractor::getUncompressedFilePath(m_directory, componentsFilename);
				if (exists(componentsFilePath) && is_regular_file(componentsFilePath))
				{			
					loginfo("found components file: [" + componentsFilePath.string() + "]");
					break;
				}
			}	

			m_path = componentsFilePath;			

			loginfo("any extraction complete, parsing data...");

			if (!std::filesystem::exists(m_path))
			{
				auto message = "components file does not exist: [" + m_path.string() + "]";				
				throw invalid_odb_error(message.c_str());
			}
			else if (!std::filesystem::is_regular_file(m_path))
			{
				auto message = "components is not a file: [" + m_path.string() + "]";
				throw invalid_odb_error(message.c_str());
			}
			
			componentsFile.open(m_path.string(), std::ios::in);
			if (!componentsFile.is_open())
			{
				auto message = "unable to open components file: [" + m_path.string() + "]";
				throw invalid_odb_error(message.c_str());
			}

			std::shared_ptr<ComponentRecord> pCurrentComponentRecord;
			std::shared_ptr<BomDescriptionRecord> pCurrentBomDescriptionRecord;
			
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

						if (pCurrentBomDescriptionRecord != nullptr)
						{
							// BomDescriptionRecord end (i.e. MPN) line
							m_bomDescriptionRecordsByCpn[pCurrentBomDescriptionRecord->cpn] = pCurrentBomDescriptionRecord;
							pCurrentBomDescriptionRecord.reset();
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
						//lineStream >> pCurrentComponentRecord->attributes;

						if (m_componentRecords.size() > UINT_MAX)
						{
							throw_parse_error(m_path, line, token, lineNumber);							
						}
						
						pCurrentComponentRecord->index = static_cast<unsigned int>(m_componentRecords.size());

						std::string attrIdString;
						lineStream >> attrIdString;

						if (!pCurrentComponentRecord->ParseAttributeLookupTable(attrIdString))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}						
					}
					else if (line.find(PropertyRecord::RECORD_TOKEN) == 0)
					{
						// component property record line
						std::string token;
						if (!(lineStream >> token))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (token != PropertyRecord::RECORD_TOKEN)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						auto pPropertyRecord = std::make_shared<PropertyRecord>();

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

						double f;
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
							logdebug(pi.toString("Component Toeprint record with netNumber = -1"));

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
							logdebug(pi.toString("Component Toeprint record with subnetNumber = -1"));

							if (!m_allowToepintNetNumbersOfNegative1)
							{
								throw_parse_error(m_path, line, token, lineNumber);
							}
						}

						lineStream >> pToeprintRecord->name;

						pCurrentComponentRecord->m_toeprintRecords.push_back(pToeprintRecord);
					}
					else if (line.find(ComponentsFile::BomDescriptionRecord::CPN_RECORD_TOKEN) == 0)
					{
						// BomDescriptionRecord start (i.e. CPN) line

						std::string token;
						lineStream >> token;
						if (token != ComponentsFile::BomDescriptionRecord::CPN_RECORD_TOKEN)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (pCurrentBomDescriptionRecord != nullptr)
						{
							m_bomDescriptionRecordsByCpn[pCurrentBomDescriptionRecord->cpn] = pCurrentBomDescriptionRecord;
							pCurrentBomDescriptionRecord.reset();
						}

						pCurrentBomDescriptionRecord = std::make_shared<ComponentsFile::BomDescriptionRecord>();										
					}
					else if (line.find(ComponentsFile::BomDescriptionRecord::IPN_RECORD_TOKEN) == 0)
					{
						std::string token;
						lineStream >> token;
						if (token != ComponentsFile::BomDescriptionRecord::IPN_RECORD_TOKEN)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (pCurrentBomDescriptionRecord == nullptr)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pCurrentBomDescriptionRecord->ipn))
						{
							// blank/ no IPN value allowed
							//throw_parse_error(m_path, line, token, lineNumber);
						}
					}
					else if (line.find(ComponentsFile::BomDescriptionRecord::DSC_RECORD_TOKEN) == 0)
					{
						std::string token;
						lineStream >> token;
						if (token != ComponentsFile::BomDescriptionRecord::DSC_RECORD_TOKEN)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (pCurrentBomDescriptionRecord == nullptr)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						std::string description;

						if (!(lineStream >> description))
						{
							// allow blank/no description value
							//throw_parse_error(m_path, line, token, lineNumber);
						}

						pCurrentBomDescriptionRecord->descriptions.push_back(description);
					}
					else if (line.find(ComponentsFile::BomDescriptionRecord::VPL_VND_RECORD_TOKEN) == 0)
					{
						std::string token;
						lineStream >> token;
						if (token != ComponentsFile::BomDescriptionRecord::VPL_VND_RECORD_TOKEN)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (pCurrentBomDescriptionRecord == nullptr)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pCurrentBomDescriptionRecord->vpl_vnd))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}
					}
					else if (line.find(ComponentsFile::BomDescriptionRecord::VPL_MPN_RECORD_TOKEN) == 0)
					{
						std::string token;
						lineStream >> token;
						if (token != ComponentsFile::BomDescriptionRecord::VPL_MPN_RECORD_TOKEN)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (pCurrentBomDescriptionRecord == nullptr)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pCurrentBomDescriptionRecord->vpl_mpn))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}
					}
					else if (line.find(ComponentsFile::BomDescriptionRecord::VND_RECORD_TOKEN) == 0)
					{
						std::string token;
						lineStream >> token;
						if (token != ComponentsFile::BomDescriptionRecord::VND_RECORD_TOKEN)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (pCurrentBomDescriptionRecord == nullptr)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pCurrentBomDescriptionRecord->vnd))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}
					}
					else if (line.find(ComponentsFile::BomDescriptionRecord::MPN_RECORD_TOKEN) == 0)
					{
						std::string token;
						lineStream >> token;
						if (token != ComponentsFile::BomDescriptionRecord::MPN_RECORD_TOKEN)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (pCurrentBomDescriptionRecord == nullptr)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pCurrentBomDescriptionRecord->mpn))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						// BomDescriptionRecord end (i.e. MPN) line
						//m_bomDescriptionRecordsByCpn[pCurrentBomDescriptionRecord->cpn] = pCurrentBomDescriptionRecord;
						//pCurrentBomDescriptionRecord.reset();
					}					
				}
			}

			if (pCurrentBomDescriptionRecord != nullptr)
			{
				// BomDescriptionRecord end (i.e. MPN) line
				m_bomDescriptionRecordsByCpn[pCurrentBomDescriptionRecord->cpn] = pCurrentBomDescriptionRecord;
				pCurrentBomDescriptionRecord.reset();
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