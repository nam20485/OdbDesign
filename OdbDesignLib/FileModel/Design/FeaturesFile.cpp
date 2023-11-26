#include "FeaturesFile.h"
#include "ArchiveExtractor.h"
#include <fstream>
#include "Logger.h"
#include "../invalid_odb_error.h"
#include <str_trim.h>
#include "../parse_error.h"


namespace Odb::Lib::FileModel::Design
{
	FeaturesFile::FeaturesFile()
		: m_id((unsigned) -1)
		, m_numFeatures(0)
		, m_units("")
		, m_path("")
		, m_directory("")
	{
	}

	FeaturesFile::~FeaturesFile()
	{
		m_featureRecords.clear();
		m_attributeNames.clear();
		m_attributeTextValues.clear();
	}

	bool FeaturesFile::Parse(std::filesystem::path directory)
	{
		std::ifstream featuresFile;
		int lineNumber = 0;
		std::string line;

		try
		{
			m_directory = directory;			

			loginfo("checking for extraction...");

			std::filesystem::path featuresFilePath;
			for (const std::string featuresFilename : FEATURES_FILENAMES)
			{
				loginfo("trying features file: [" + featuresFilename + "]...");

				featuresFilePath = Utils::ArchiveExtractor::getUncompressedFilePath(m_directory, featuresFilename);
				if (exists(featuresFilePath) && is_regular_file(featuresFilePath))
				{
					loginfo("found features file: [" + featuresFilePath.string() + "]");
					break;
				}
			}

			m_path = featuresFilePath;

			loginfo("any extraction complete, parsing data...");

			if (!std::filesystem::exists(m_path))
			{
				auto message = "features file does not exist: [" + m_path.string() + "]";
				throw invalid_odb_error(message.c_str());
			}
			else if (!std::filesystem::is_regular_file(m_path))
			{
				auto message = "features is not a file: [" + m_path.string() + "]";
				throw invalid_odb_error(message.c_str());
			}

			featuresFile.open(m_path.string(), std::ios::in);
			if (!featuresFile.is_open())
			{
				auto message = "unable to open features file: [" + m_path.string() + "]";
				throw invalid_odb_error(message.c_str());
			}

			std::shared_ptr<FeatureRecord> pCurrentFeatureRecord;

			while (std::getline(featuresFile, line))
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
					else if (line.find(NUM_FEATURES_TOKEN) == 0)
					{
						// component record line
						std::string token;

						if (!(lineStream >> token))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}
						
						if (token != NUM_FEATURES_TOKEN)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> m_numFeatures))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}
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
					else
					{
						//auto message = "unknown line: [" + line + "]";
						//throw invalid_odb_error(message.c_str());
					}
				}
			}

			// do we have a current feature record? (finish up the last record in the file- i.e. ran out of file)
			if (pCurrentFeatureRecord != nullptr)
			{
				// finish it up and add it to the list
				m_featureRecords.push_back(pCurrentFeatureRecord);
				pCurrentFeatureRecord.reset();
			}

			featuresFile.close();
		}
		catch (parse_error& pe)
		{
			auto m = pe.toString("Parse Error:");
			logerror(m);
			featuresFile.close();
			throw pe;
		}
		catch (std::exception& e)
		{
			parse_info pi(m_path, line, lineNumber);
			const auto m = pi.toString();
			logexception_msg(e, m);
			featuresFile.close();
			throw e;
		}

		return true;
	}

	std::string FeaturesFile::GetUnits() const
	{
		return m_units;
	}

	std::filesystem::path FeaturesFile::GetPath()
	{
		return m_path;
	}

	std::filesystem::path FeaturesFile::GetDirectory()
	{
		return m_directory;
	}

	int FeaturesFile::GetNumFeatures() const
	{
		return m_numFeatures;
	}

	unsigned int FeaturesFile::GetId() const
	{
		return m_id;
	}

	const FeaturesFile::FeatureRecord::Vector& FeaturesFile::GetFeatureRecords() const
	{
		return m_featureRecords;
	}

	FeaturesFile::FeatureRecord::~FeatureRecord()
	{
		m_contourPolygons.clear();
	}

	const ContourPolygon::Vector& FeaturesFile::FeatureRecord::GetContourPolygons() const
	{
		return m_contourPolygons;
	}
}