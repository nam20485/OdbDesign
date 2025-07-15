//
// Created by nmill on 10/13/2023.
//

#include "AttrListFile.h"
#include <string>
#include <fstream>
#include "../parse_error.h"
#include "../invalid_odb_error.h"
#include "Logger.h"
#include "str_utils.h"
#include "../../Constants.h"
#include <ArchiveExtractor.h>

namespace Odb::Lib::FileModel::Design
{
    AttrListFile::AttrListFile()
        : m_directory("")
		, m_path("")
		, m_units("")
    {
    }

    std::string AttrListFile::GetUnits() const
    {
        return m_units;
    }

    const AttrListFile::AttributeMap& AttrListFile::GetAttributes() const
    {
        return m_attributesByName;
    }

    bool Lib::FileModel::Design::AttrListFile::Parse(std::filesystem::path directory)
    {
		std::ifstream attrListFile;
		int lineNumber = 0;
		std::string line;

		try
		{
			m_directory = directory;

			loginfo("checking for extraction...");

			std::filesystem::path attrListFilePath;
			for (const std::string attrListFilename : ATTRLIST_FILENAMES)
			{
				loginfo("trying attrlist file: [" + attrListFilename + "]...");

				attrListFilePath = Utils::ArchiveExtractor::getUncompressedFilePath(m_directory, attrListFilename);
				if (exists(attrListFilePath) && is_regular_file(attrListFilePath))
				{
					loginfo("found attrlist file: [" + attrListFilePath.string() + "]");
					break;
				}
			}

			m_path = attrListFilePath;

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

			attrListFile.open(m_path.string(), std::ios::in);
			if (!attrListFile.is_open())
			{
				auto message = "unable to open attrlist file: [" + m_path.string() + "]";
				throw invalid_odb_error(message.c_str());
			}
			
			while (std::getline(attrListFile, line))
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
					else if (line.find(Constants::UNITS_TOKEN) == 0)
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
					else
					{
						// attribute line
						std::string attribute;
						std::string value;

						if (!std::getline(lineStream, attribute, '='))
						{
							throw_parse_error(m_path, line, "", lineNumber);
						}

						if (!std::getline(lineStream, value))
						{
							if (!attributeValueIsOptional(attribute))
							{
								logwarn("attrlist file: no value for non-optional attribute: " + attribute);
							}

						}

						Utils::str_trim(attribute);
						Utils::str_trim(value);

						m_attributesByName[attribute] = value;
					}
				}
			}			

			attrListFile.close();
		}
		catch (parse_error& pe)
		{
			auto m = pe.toString("Parse Error:");
			logerror(m);
			attrListFile.close();
			throw pe;
		}
		catch (std::exception& e)
		{
			parse_info pi(m_path, line, lineNumber);
			const auto m = pi.toString();
			logexception_msg(e, m);
			attrListFile.close();
			throw e;
		}

		return true;
    }

	std::unique_ptr<Odb::Lib::Protobuf::AttrListFile> Lib::FileModel::Design::AttrListFile::to_protobuf() const
	{
		auto message = std::make_unique<Odb::Lib::Protobuf::AttrListFile>();

		message->set_directory(m_directory.string());
		message->set_path(m_path.string());
		message->set_units(m_units);

		for (const auto& kvAttribute : m_attributesByName)
		{
			(*message->mutable_attributesbyname())[kvAttribute.first] = kvAttribute.second;
		}

		return message;
	
	}

	void Lib::FileModel::Design::AttrListFile::from_protobuf(const Odb::Lib::Protobuf::AttrListFile& message)
	{
		m_directory = message.directory();
		m_path = message.path();
		m_units = message.units();

		for (const auto& kvAttribute : message.attributesbyname())
		{
			m_attributesByName[kvAttribute.first] = kvAttribute.second;
		}
	}

	bool Lib::FileModel::Design::AttrListFile::attributeValueIsOptional(const std::string& attributeName) const
	{
		//for (const auto& optionalAttribute : OPTIONAL_ATTRIBUTES)
		//{
		//	if (attributeName == optionalAttribute)
		//	{
		//		return true;
		//	}
		//}
		//return false;

		// all atrributes are "optional", i.e. value is not required
		return true;
	}

	bool AttrListFile::Save(std::ostream& os)
	{
		os << Constants::UNITS_TOKEN << " = " << m_units << std::endl;
		for (const auto& kvAttribute : m_attributesByName)
		{
			os << kvAttribute.first << " = " << kvAttribute.second << std::endl;
		}

		return true;
	}
} // namespace Odb::Lib::FileModel::Design