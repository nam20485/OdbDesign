#include "FeaturesFile.h"
#include "ArchiveExtractor.h"
#include <fstream>
#include "Logger.h"
#include "../invalid_odb_error.h"
#include <str_trim.h>
#include "../parse_error.h"
#include "SymbolName.h"


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
		m_symbolNames.clear();
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
			std::shared_ptr<ContourPolygon> pCurrentContourPolygon;

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
					else if (line.find("U") == 0)
					{
						std::string token;
						if (!std::getline(lineStream, token, ' '))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}
						else if (!std::getline(lineStream, token, ' '))
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
					else if (line.find(SYMBOL_NAME_TOKEN) == 0)
					{
						// component attribute text string values	
						auto pSymbolName = std::make_shared<SymbolName>();						
						if (!pSymbolName->SymbolName::Parse(m_path, line, lineNumber))
						{
							throw_parse_error(m_path, line, "", lineNumber);
						}
						m_symbolNames.push_back(pSymbolName);
					}
					else if (line.find(FeatureRecord::LINE_TOKEN) == 0)
					{
						std::string token;
						if (!(lineStream >> token) || token != FeatureRecord::LINE_TOKEN)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						auto pFeatureRecord = std::make_shared<FeatureRecord>();
						pFeatureRecord->type = FeatureRecord::Type::Line;

						if (!(lineStream >> pFeatureRecord->xs))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pFeatureRecord->ys))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pFeatureRecord->xe))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pFeatureRecord->ye))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pFeatureRecord->sym_num))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						char polarity;
						if (!(lineStream >> polarity))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}
						switch (polarity)
						{
						case 'P': pFeatureRecord->polarity = Polarity::Positive; break;
						case 'N': pFeatureRecord->polarity = Polarity::Negative; break;
						default: throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pFeatureRecord->dcode))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						lineStream >> pFeatureRecord->attributesIdString;

						m_featureRecords.push_back(pFeatureRecord);
					}
					else if (line.find(FeatureRecord::PAD_TOKEN) == 0)
					{
						std::string token;
						if (!(lineStream >> token) || token != FeatureRecord::PAD_TOKEN)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						auto pFeatureRecord = std::make_shared<FeatureRecord>();
						pFeatureRecord->type = FeatureRecord::Type::Pad;

						if (!(lineStream >> pFeatureRecord->x))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pFeatureRecord->y))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pFeatureRecord->apt_def_symbol_num))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}
						if (pFeatureRecord->apt_def_symbol_num == -1)
						{
							if (!(lineStream >> pFeatureRecord->apt_def_resize_factor))
							{
								throw_parse_error(m_path, line, token, lineNumber);
							}
						}

						char polarity;
						if (!(lineStream >> polarity))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}
						switch (polarity)
						{
						case 'P': pFeatureRecord->polarity = Polarity::Positive; break;
						case 'N': pFeatureRecord->polarity = Polarity::Negative; break;
						default: throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pFeatureRecord->dcode))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pFeatureRecord->orient_def))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						lineStream >> pFeatureRecord->attributesIdString;

						m_featureRecords.push_back(pFeatureRecord);
					}
					else if (line.find(FeatureRecord::TEXT_TOKEN) == 0)
					{
						std::string token;
						if (!(lineStream >> token) || token != FeatureRecord::TEXT_TOKEN)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						auto pFeatureRecord = std::make_shared<FeatureRecord>();
						pFeatureRecord->type = FeatureRecord::Type::Text;

						if (!(lineStream >> pFeatureRecord->x))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pFeatureRecord->y))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pFeatureRecord->font))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						char polarity;
						if (!(lineStream >> polarity))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}
						switch (polarity)
						{
						case 'P': pFeatureRecord->polarity = Polarity::Positive; break;
						case 'N': pFeatureRecord->polarity = Polarity::Negative; break;
						default: throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pFeatureRecord->orient_def))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (pFeatureRecord->orient_def == 8 ||
							pFeatureRecord->orient_def == 9)
						{
							if (!(lineStream >> pFeatureRecord->orient_def_rotation))
							{
								throw_parse_error(m_path, line, token, lineNumber);
							}
						}

						if (!(lineStream >> pFeatureRecord->xsize))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pFeatureRecord->ysize))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pFeatureRecord->width_factor))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pFeatureRecord->text))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pFeatureRecord->version))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}						

						lineStream >> pFeatureRecord->attributesIdString;

						m_featureRecords.push_back(pFeatureRecord);
					}					
					else if (line.find(FeatureRecord::ARC_TOKEN) == 0)
					{
						std::string token;
						if (!(lineStream >> token) || token != FeatureRecord::ARC_TOKEN)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						auto pFeatureRecord = std::make_shared<FeatureRecord>();
						pFeatureRecord->type = FeatureRecord::Type::Arc;

						if (!(lineStream >> pFeatureRecord->xs))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pFeatureRecord->ys))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pFeatureRecord->xe))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pFeatureRecord->ye))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pFeatureRecord->xc))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pFeatureRecord->yc))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}
						

						if (!(lineStream >> pFeatureRecord->sym_num))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						char polarity;
						if (!(lineStream >> polarity))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}
						switch (polarity)
						{
						case 'P': pFeatureRecord->polarity = Polarity::Positive; break;
						case 'N': pFeatureRecord->polarity = Polarity::Negative; break;
						default: throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pFeatureRecord->dcode))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						char cw;
						if (!(lineStream >> cw))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}
						switch (cw)
						{
						case 'Y': pFeatureRecord->cw = true; break;
						case 'N': pFeatureRecord->cw = false; break;
						default: throw_parse_error(m_path, line, token, lineNumber);
						}						

						lineStream >> pFeatureRecord->attributesIdString;

						m_featureRecords.push_back(pFeatureRecord);
					}
					else if (line.find(FeatureRecord::BARCODE_TOKEN) == 0)
					{
						std::string token;
						if (!(lineStream >> token) || token != FeatureRecord::BARCODE_TOKEN)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						auto pFeatureRecord = std::make_shared<FeatureRecord>();
						pFeatureRecord->type = FeatureRecord::Type::Barcode;
						
						// TODO: barcode feature record type

						m_featureRecords.push_back(pFeatureRecord);						
					}
					else if (line.find(FeatureRecord::SURFACE_START_TOKEN) == 0 &&
							 line.size() > 1 && line[1] == ' ')
					{
						std::string token;
						if (!(lineStream >> token) || token != FeatureRecord::SURFACE_START_TOKEN)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						pCurrentFeatureRecord = std::make_shared<FeatureRecord>();
						pCurrentFeatureRecord->type = FeatureRecord::Type::Surface;

						char polarity;
						if (!(lineStream >> polarity))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}
						switch (polarity)
						{
						case 'P': pCurrentFeatureRecord->polarity = Polarity::Positive; break;
						case 'N': pCurrentFeatureRecord->polarity = Polarity::Negative; break;
						default: throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pCurrentFeatureRecord->dcode))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}						

						lineStream >> pCurrentFeatureRecord->attributesIdString;
					}
					else if (line.find(FeatureRecord::SURFACE_END_TOKEN) == 0)
					{
						std::string token;
						if (!(lineStream >> token) || token != FeatureRecord::SURFACE_END_TOKEN)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (pCurrentFeatureRecord != nullptr)
						{
							m_featureRecords.push_back(pCurrentFeatureRecord);
							pCurrentFeatureRecord.reset();
						}
						else
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}
					}
					else if (line.find(ContourPolygon::BEGIN_RECORD_TOKEN) == 0)
					{
						std::string token;
						if (!(lineStream >> token))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (token != ContourPolygon::BEGIN_RECORD_TOKEN)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						pCurrentContourPolygon = std::make_shared<ContourPolygon>();

						if (!(lineStream >> pCurrentContourPolygon->xStart))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pCurrentContourPolygon->yStart))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> token))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (token == ContourPolygon::ISLAND_TYPE_TOKEN)
						{
							pCurrentContourPolygon->type = ContourPolygon::Type::Island;
						}
						else if (token == ContourPolygon::HOLE_TYPE_TOKEN)
						{
							pCurrentContourPolygon->type = ContourPolygon::Type::Hole;
						}
						else
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}
					}
					else if (line.find(ContourPolygon::END_RECORD_TOKEN) == 0)
					{
						std::string token;
						if (!(lineStream >> token))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (token != ContourPolygon::END_RECORD_TOKEN)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (pCurrentFeatureRecord != nullptr)
						{
							pCurrentFeatureRecord->m_contourPolygons.push_back(pCurrentContourPolygon);
							pCurrentContourPolygon.reset();
						}
						else
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}
					}
					else if (line.find(ContourPolygon::PolygonPart::ARC_RECORD_TOKEN) == 0)
					{
						std::string token;
						if (!(lineStream >> token))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (token != ContourPolygon::PolygonPart::ARC_RECORD_TOKEN)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						auto pPolygonPart = std::make_shared<ContourPolygon::PolygonPart>();
						pPolygonPart->type = ContourPolygon::PolygonPart::Type::Arc;

						if (!(lineStream >> pPolygonPart->endX))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pPolygonPart->endY))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pPolygonPart->xCenter))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pPolygonPart->yCenter))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> token))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (token == "y" || token == "Y")
						{
							pPolygonPart->isClockwise = true;
						}
						else if (token == "n" || token == "N")
						{
							pPolygonPart->isClockwise = false;
						}
						else
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (pCurrentContourPolygon != nullptr)
						{
							pCurrentContourPolygon->m_polygonParts.push_back(pPolygonPart);
						}
						else
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}
					}
					else if (line.find(ContourPolygon::PolygonPart::SEGMENT_RECORD_TOKEN) == 0)
					{
						std::string token;
						if (!(lineStream >> token))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (token != ContourPolygon::PolygonPart::SEGMENT_RECORD_TOKEN)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						auto pPolygonPart = std::make_shared<ContourPolygon::PolygonPart>();
						pPolygonPart->type = ContourPolygon::PolygonPart::Type::Segment;

						if (!(lineStream >> pPolygonPart->endX))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pPolygonPart->endY))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (pCurrentContourPolygon != nullptr)
						{
							pCurrentContourPolygon->m_polygonParts.push_back(pPolygonPart);
						}
						else
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}
					}
					else
					{
						// unrecognized record line
						parse_info pi(m_path, line, lineNumber);
						logwarn(pi.toString("unrecognized record line in features file:"));
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

	const SymbolName::Vector& FeaturesFile::GetSymbolNames() const
	{
		return m_symbolNames;
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