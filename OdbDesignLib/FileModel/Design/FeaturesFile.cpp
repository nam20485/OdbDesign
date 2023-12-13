#include "FeaturesFile.h"
#include "FeaturesFile.h"
#include "FeaturesFile.h"
#include "FeaturesFile.h"
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

	std::unique_ptr<Odb::Lib::Protobuf::FeaturesFile> FeaturesFile::to_protobuf() const
	{
		std::unique_ptr<Odb::Lib::Protobuf::FeaturesFile> pFeaturesFileMessage(new Odb::Lib::Protobuf::FeaturesFile);
		pFeaturesFileMessage->set_id(m_id);
		pFeaturesFileMessage->set_numfeatures(m_numFeatures);
		pFeaturesFileMessage->set_units(m_units);
		for (const auto& pFeatureRecord : m_featureRecords)
		{
			pFeaturesFileMessage->add_featurerecords()->CopyFrom(*pFeatureRecord->to_protobuf());
		}
		return pFeaturesFileMessage;
	}

	void FeaturesFile::from_protobuf(const Odb::Lib::Protobuf::FeaturesFile& message)
	{
		m_id = message.id();
		m_numFeatures = message.numfeatures();
		m_units = message.units();
		for (const auto& featureRecordMessage : message.featurerecords())
		{
			std::shared_ptr<FeatureRecord> pFeatureRecord(new FeatureRecord);
			pFeatureRecord->from_protobuf(featureRecordMessage);
			m_featureRecords.push_back(pFeatureRecord);
		}
	}

	FeaturesFile::FeatureRecord::~FeatureRecord()
	{
		m_contourPolygons.clear();
	}

	const ContourPolygon::Vector& FeaturesFile::FeatureRecord::GetContourPolygons() const
	{
		return m_contourPolygons;
	}

	std::unique_ptr<Odb::Lib::Protobuf::FeaturesFile::FeatureRecord> Odb::Lib::FileModel::Design::FeaturesFile::FeatureRecord::to_protobuf() const
	{
		std::unique_ptr<Odb::Lib::Protobuf::FeaturesFile::FeatureRecord> pFeatureRecordMessage(new Odb::Lib::Protobuf::FeaturesFile::FeatureRecord);
		pFeatureRecordMessage->set_apt_def(apt_def);
		pFeatureRecordMessage->set_apt_def_resize_factor(apt_def_resize_factor);
		pFeatureRecordMessage->set_xc(xc);
		pFeatureRecordMessage->set_yc(yc);
		pFeatureRecordMessage->set_cw(cw);
		pFeatureRecordMessage->set_font(font);
		pFeatureRecordMessage->set_xsize(xsize);
		pFeatureRecordMessage->set_ysize(ysize);
		pFeatureRecordMessage->set_width_factor(width_factor);
		pFeatureRecordMessage->set_text(text);
		pFeatureRecordMessage->set_version(version);
		pFeatureRecordMessage->set_sym_num(sym_num);
		pFeatureRecordMessage->set_polarity(static_cast<Odb::Lib::Protobuf::Polarity>(polarity));
		pFeatureRecordMessage->set_dcode(dcode);
		pFeatureRecordMessage->set_atr(atr);
		pFeatureRecordMessage->set_value(value);
		pFeatureRecordMessage->set_id(id);
		pFeatureRecordMessage->set_orient_def(orient_def);
		pFeatureRecordMessage->set_orient_def_rotation(orient_def_rotation);
		pFeatureRecordMessage->set_type(static_cast<Odb::Lib::Protobuf::FeaturesFile::FeatureRecord::Type>(type));
		pFeatureRecordMessage->set_xs(xs);
		pFeatureRecordMessage->set_ys(ys);
		pFeatureRecordMessage->set_xe(xe);
		pFeatureRecordMessage->set_ye(ye);
		pFeatureRecordMessage->set_x(x);
		pFeatureRecordMessage->set_y(y);
		pFeatureRecordMessage->set_apt_def_symbol_num(apt_def_symbol_num);
		for (const auto& pContourPolygon : m_contourPolygons)
		{
			pFeatureRecordMessage->add_contourpolygons()->CopyFrom(*pContourPolygon->to_protobuf());
		}
		return pFeatureRecordMessage;
	}

	void Odb::Lib::FileModel::Design::FeaturesFile::FeatureRecord::from_protobuf(const Odb::Lib::Protobuf::FeaturesFile::FeatureRecord& message)
	{
		apt_def = message.apt_def();
		apt_def_resize_factor = message.apt_def_resize_factor();
		xc = message.xc();
		yc = message.yc();
		cw = message.cw();
		font = message.font();
		xsize = message.xsize();
		ysize = message.ysize();
		width_factor = message.width_factor();
		text = message.text();
		version = message.version();
		sym_num = message.sym_num();
		polarity = static_cast<Polarity>(message.polarity());
		dcode = message.dcode();
		atr = message.atr();
		value = message.value();
		id = message.id();
		orient_def = message.orient_def();
		orient_def_rotation = message.orient_def_rotation();
		type = static_cast<Type>(message.type());
		xs = message.xs();
		ys = message.ys();
		xe = message.xe();
		ye = message.ye();
		x = message.x();
		y = message.y();
		apt_def_symbol_num = message.apt_def_symbol_num();
		for (const auto& contourPolygonMessage : message.contourpolygons())
		{
			std::shared_ptr<ContourPolygon> pContourPolygon(new ContourPolygon);
			pContourPolygon->from_protobuf(contourPolygonMessage);
			m_contourPolygons.push_back(pContourPolygon);
		}
	}
}