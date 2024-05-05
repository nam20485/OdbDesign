#include "EdaDataFile.h"
#include <fstream>
#include <sstream>
#include "str_utils.h"
#include "../../enums.h"
#include "../../ProtoBuf/edadatafile.pb.h"
#include "ArchiveExtractor.h"
#include "Logger.h"
#include "../parse_info.h"
#include "../parse_error.h"
#include "../invalid_odb_error.h"

using namespace std::filesystem;
using namespace Utils;

namespace Odb::Lib::FileModel::Design
{
    EdaDataFile::EdaDataFile(bool logAllLineParsing)
        : m_logAllLineParsing(logAllLineParsing)
    {
    }
   
    EdaDataFile::~EdaDataFile()
    {
        m_layerNames.clear();
        m_attributeNames.clear();
        m_attributeTextValues.clear();
        m_netRecords.clear();
        m_netRecordsByName.clear();
        m_packageRecords.clear();
        m_packageRecordsByName.clear();
        m_featureGroupRecords.clear();
        m_propertyRecords.clear();
    }

    const std::filesystem::path& EdaDataFile::GetPath() const
    {
        return m_path;
    }

    const std::filesystem::path& EdaDataFile::GetDirectory() const
    {
        return m_directory;
    }

    const std::string& EdaDataFile::GetUnits() const
    {
        return m_units;
    }

    const std::string& EdaDataFile::GetSource() const
    {
        return m_source;
    }

    EdaDataFile::NetRecord::SubnetRecord::~SubnetRecord()
    {
        m_featureIdRecords.clear();
    }

    std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile::NetRecord::SubnetRecord> EdaDataFile::NetRecord::SubnetRecord::to_protobuf() const
    {
        std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile::NetRecord::SubnetRecord> pSubnetRecordMessage(new Odb::Lib::Protobuf::EdaDataFile::NetRecord::SubnetRecord);       
        pSubnetRecordMessage->set_type((Odb::Lib::Protobuf::EdaDataFile::NetRecord::SubnetRecord::Type) type);
        if (type == Type::Toeprint)
        {
            pSubnetRecordMessage->set_componentnumber(componentNumber);
            pSubnetRecordMessage->set_toeprintnumber(toeprintNumber);
            pSubnetRecordMessage->set_side((Odb::Lib::Protobuf::BoardSide)side);
        }
        else if (type == Type::Plane)
        {
            pSubnetRecordMessage->set_filltype((Odb::Lib::Protobuf::EdaDataFile::NetRecord::SubnetRecord::FillType)fillType);
            pSubnetRecordMessage->set_cutouttype((Odb::Lib::Protobuf::EdaDataFile::NetRecord::SubnetRecord::CutoutType) cutoutType);
            pSubnetRecordMessage->set_fillsize(fillSize);
        }

        for (const auto& featureIdRecord : m_featureIdRecords)
        {
            auto pFeatureIdRecordMessage = pSubnetRecordMessage->add_featureidrecords();
            pFeatureIdRecordMessage->CopyFrom(*featureIdRecord->to_protobuf());
        }
        return pSubnetRecordMessage;
    }

    void EdaDataFile::NetRecord::SubnetRecord::from_protobuf(const Odb::Lib::Protobuf::EdaDataFile::NetRecord::SubnetRecord& message)
    {
		type = static_cast<Type>(message.type());
       
		if (type == Type::Toeprint)
		{
			componentNumber = message.componentnumber();
			toeprintNumber = message.toeprintnumber();
			side = static_cast<BoardSide>(message.side());
		}
		else if (type == Type::Plane)
		{
			fillType = static_cast<FillType>(message.filltype());
			cutoutType = static_cast<CutoutType>(message.cutouttype());
			fillSize = message.fillsize();
            index = message.index();
		}

        for (const auto& featureIdRecordMessage : message.featureidrecords())
        {
            auto pFeatureIdRecord = std::make_shared<FeatureIdRecord>();
            pFeatureIdRecord->from_protobuf(featureIdRecordMessage);
            m_featureIdRecords.push_back(pFeatureIdRecord);
        }
    }

    EdaDataFile::NetRecord::~NetRecord()
    {
        m_subnetRecords.clear();
        m_propertyRecords.clear();
    }

    std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile::NetRecord> EdaDataFile::NetRecord::to_protobuf() const
    {
        std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile::NetRecord> pNetRecordMessage(new Odb::Lib::Protobuf::EdaDataFile::NetRecord);        
        pNetRecordMessage->set_name(name);
        //pNetRecordMessage->set_attributesidstring(attributesIdString);
        pNetRecordMessage->set_index(index);

        for (const auto& propertyRecord : m_propertyRecords)
        {
            auto pPropertyRecordMessage = pNetRecordMessage->add_propertyrecords();
            pPropertyRecordMessage->CopyFrom(*propertyRecord->to_protobuf());
        }

        for (const auto& subnetRecord : m_subnetRecords)
        {
			auto pSubnetRecordMessage = pNetRecordMessage->add_subnetrecords();
			pSubnetRecordMessage->CopyFrom(*subnetRecord->to_protobuf());
        }   
        for (const auto& kvAttributeAssignment : m_attributeLookupTable)
        {
            (*pNetRecordMessage->mutable_attributelookuptable())[kvAttributeAssignment.first] = kvAttributeAssignment.second;
        }
        return pNetRecordMessage;
    }

    void EdaDataFile::NetRecord::from_protobuf(const Odb::Lib::Protobuf::EdaDataFile::NetRecord& message)
    {
        name = message.name();
		//attributesIdString = message.attributesidstring();
		index = message.index();

		for (const auto& propertyRecordMessage : message.propertyrecords())
		{
			auto pPropertyRecord = std::make_shared<PropertyRecord>();
			pPropertyRecord->from_protobuf(propertyRecordMessage);
			m_propertyRecords.push_back(pPropertyRecord);
		}

		for (const auto& subnetRecordMessage : message.subnetrecords())
		{
			auto pSubnetRecord = std::make_shared<SubnetRecord>();
			pSubnetRecord->from_protobuf(subnetRecordMessage);
			m_subnetRecords.push_back(pSubnetRecord);
		}   

        for (const auto& kvAttributeAssignment : message.attributelookuptable())
        {
            m_attributeLookupTable[kvAttributeAssignment.first] = kvAttributeAssignment.second;
        }
    }

    std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile::FeatureIdRecord>
    EdaDataFile::FeatureIdRecord::to_protobuf() const
    {
        std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile::FeatureIdRecord> pFeatureIdRecordMessage(new Odb::Lib::Protobuf::EdaDataFile::FeatureIdRecord);        
        pFeatureIdRecordMessage->set_type((Odb::Lib::Protobuf::EdaDataFile::FeatureIdRecord::Type) type);
        pFeatureIdRecordMessage->set_layernumber(layerNumber);
        pFeatureIdRecordMessage->set_featurenumber(featureNumber);
        return pFeatureIdRecordMessage;
    }

    void EdaDataFile::FeatureIdRecord::from_protobuf(const Odb::Lib::Protobuf::EdaDataFile::FeatureIdRecord& message)
    {
        type = static_cast<Type>(message.type());
        layerNumber = message.layernumber();
        featureNumber = message.featurenumber();
    }   

    std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile::FeatureGroupRecord>
    EdaDataFile::FeatureGroupRecord::to_protobuf() const
    {
        std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile::FeatureGroupRecord> pFeatureGroupRecordMessage(new Odb::Lib::Protobuf::EdaDataFile::FeatureGroupRecord);
        pFeatureGroupRecordMessage->set_type(type);
        for (const auto& featureIdRecord : m_featureIdRecords)
        {
            auto pFeatureIdRecordMessage = pFeatureGroupRecordMessage->add_featureidrecords();
            pFeatureIdRecordMessage->CopyFrom(*featureIdRecord->to_protobuf());
        }
        for (const auto& propertyRecord : m_propertyRecords)
        {
            auto pPropertyRecordMessage = pFeatureGroupRecordMessage->add_featureidrecords();
            pPropertyRecordMessage->CopyFrom(*propertyRecord->to_protobuf());
        }
        return pFeatureGroupRecordMessage;
    }

    void EdaDataFile::FeatureGroupRecord::from_protobuf(const Odb::Lib::Protobuf::EdaDataFile::FeatureGroupRecord& message)
    {
        type = message.type();
        for (const auto& featureIdRecordMessage : message.featureidrecords())
		{
			auto pFeatureIdRecord = std::make_shared<FeatureIdRecord>();
			pFeatureIdRecord->from_protobuf(featureIdRecordMessage);
			m_featureIdRecords.push_back(pFeatureIdRecord);
		}
        for (const auto& propertyRecordMessage : message.propertyrecords())
        {
            auto pPropertyRecord = std::make_shared<PropertyRecord>();
			pPropertyRecord->from_protobuf(propertyRecordMessage);
			m_propertyRecords.push_back(pPropertyRecord);
		}
    }    

    const std::vector<std::string>& EdaDataFile::GetLayerNames() const
    {
        return m_layerNames;
    }

    const std::vector<std::string>& EdaDataFile::GetAttributeNames() const
    {
        return m_attributeNames;
    }

    const std::vector<std::string>& EdaDataFile::GetAttributeTextValues() const
    {
        return m_attributeTextValues;
    }

    const EdaDataFile::NetRecord::Vector& EdaDataFile::GetNetRecords() const
    {
        return m_netRecords;
    }

    const EdaDataFile::NetRecord::StringMap& EdaDataFile::GetNetRecordsByName() const
    {
        return m_netRecordsByName;
    }

    const EdaDataFile::PackageRecord::Vector& EdaDataFile::GetPackageRecords() const
    {
        return m_packageRecords;
    }

    const EdaDataFile::PackageRecord::StringMap& EdaDataFile::GetPackageRecordsByName() const
    {
        return m_packageRecordsByName;
    }

    const EdaDataFile::FeatureGroupRecord::Vector& EdaDataFile::GetFeatureGroupRecords() const
    {
        return m_featureGroupRecords;
    }

    const PropertyRecord::Vector& EdaDataFile::GetPropertyRecords() const
    {
        return m_propertyRecords;
    }

    std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile> EdaDataFile::to_protobuf() const
    {
        std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile> pEdaDataFileMessage(new Odb::Lib::Protobuf::EdaDataFile);                
        pEdaDataFileMessage->set_path(m_path.string());
        pEdaDataFileMessage->set_units(m_units);
        pEdaDataFileMessage->set_source(m_source);
        for (const auto& layerName : m_layerNames)
        {
			pEdaDataFileMessage->add_layernames(layerName);
		}
        for (const auto& attributeName : m_attributeNames)
        {
            pEdaDataFileMessage->add_attributenames(attributeName);
        }
        for (const auto& attrTextValue : m_attributeTextValues)
        {
            pEdaDataFileMessage->add_attributetextvalues(attrTextValue);
        }
        for (const auto& pNetRecord : m_netRecords)
        {
            auto pNetRecordMessage = pEdaDataFileMessage->add_netrecords();
            pNetRecordMessage->CopyFrom(*pNetRecord->to_protobuf());
        }              
        for (const auto& kvNetRecord : m_netRecordsByName)
        {
            (*pEdaDataFileMessage->mutable_netrecordsbyname())[kvNetRecord.first] = *kvNetRecord.second->to_protobuf();
        }
        for (const auto& pPackageRecord : m_packageRecords)
        {
            auto pPackageRecordMessage = pEdaDataFileMessage->add_packagerecords();
            pPackageRecordMessage->CopyFrom(*pPackageRecord->to_protobuf());
        }
        for (const auto& kvPackageRecord : m_packageRecordsByName)
        {
            (*pEdaDataFileMessage->mutable_packagerecordsbyname())[kvPackageRecord.first] = *kvPackageRecord.second->to_protobuf();
        }   
        for (const auto& pPropertyRecord : m_propertyRecords)
        {
            auto pPropertyRecordMessage = pEdaDataFileMessage->add_propertyrecords();
            pPropertyRecordMessage->CopyFrom(*pPropertyRecord->to_protobuf());
        }
        for (const auto& pFeatureGroupRecord : m_featureGroupRecords)
        {
            auto pFeatureGroupRecordMessage = pEdaDataFileMessage->add_featuregrouprecords();
            pFeatureGroupRecordMessage->CopyFrom(*pFeatureGroupRecord->to_protobuf());
        }
       
        return pEdaDataFileMessage;
    }

    void EdaDataFile::from_protobuf(const Odb::Lib::Protobuf::EdaDataFile& message)
    {
        m_path = message.path();
		m_units = message.units();
		m_source = message.source();
		for (const auto& layerName : message.layernames())
		{
			m_layerNames.push_back(layerName);
		}
		for (const auto& attributeName : message.attributenames())
		{
			m_attributeNames.push_back(attributeName);
		}
		for (const auto& attrTextValue : message.attributetextvalues())
		{
			m_attributeTextValues.push_back(attrTextValue);
		}
		for (const auto& netRecordMessage : message.netrecords())
		{
			auto pNetRecord = std::make_shared<NetRecord>();
			pNetRecord->from_protobuf(netRecordMessage);
			m_netRecords.push_back(pNetRecord);
		}
		for (const auto& kvNetRecordMessage : message.netrecordsbyname())
		{
			auto pNetRecord = std::make_shared<NetRecord>();
			pNetRecord->from_protobuf(kvNetRecordMessage.second);
			m_netRecordsByName[kvNetRecordMessage.first] = pNetRecord;
		}
		for (const auto& packageRecordMessage : message.packagerecords())
		{
			auto pPackageRecord = std::make_shared<PackageRecord>();
			pPackageRecord->from_protobuf(packageRecordMessage);
			m_packageRecords.push_back(pPackageRecord);
		}
		for (const auto& kvPackageRecordMessage : message.packagerecordsbyname())
		{
			auto pPackageRecord = std::make_shared<PackageRecord>();
			pPackageRecord->from_protobuf(kvPackageRecordMessage.second);
			m_packageRecordsByName[kvPackageRecordMessage.first] = pPackageRecord;
		}
		for (const auto& propertyRecordMessage : message.propertyrecords())
		{
			auto pPropertyRecord = std::make_shared<PropertyRecord>();
			pPropertyRecord->from_protobuf(propertyRecordMessage);
			m_propertyRecords.push_back(pPropertyRecord);
		}
		for (const auto& featureGroupRecordMessage : message.featuregrouprecords())
		{
			auto pFeatureGroupRecord = std::make_shared<FeatureGroupRecord>();
			pFeatureGroupRecord->from_protobuf(featureGroupRecordMessage);
			m_featureGroupRecords.push_back(pFeatureGroupRecord);
		}       
    }

    bool EdaDataFile::Save(std::ostream& os)
    {
        return true;
    }

    bool EdaDataFile::Parse(std::filesystem::path path)
    {
        std::ifstream edaDataFile;
        int lineNumber = 0;
        std::string line;

        try
        {
            loginfo("checking for extraction...");

            m_directory = path;
            m_path = ArchiveExtractor::getUncompressedFilePath(m_directory.string(), EDADATA_FILENAME);

            if (!std::filesystem::exists(m_path))
            {
                auto message = "eda/data file does not exist: [" + m_path.string() + "]";
                throw invalid_odb_error(message.c_str());
            }
            else if (!std::filesystem::is_regular_file(m_path))
            {
                auto message = "eda/data is not a file: [" + m_path.string() + "]";
                throw invalid_odb_error(message.c_str());
            }

            loginfo("any extraction complete, parsing data...");
            
            edaDataFile.open(m_path.string(), std::ios::in);
            if (!edaDataFile.is_open())
            {
                auto message = "unable to open eda/data file: [" + m_path.string() + "]";
                throw invalid_odb_error(message.c_str());
            }

            std::shared_ptr<NetRecord> pCurrentNetRecord;
            std::shared_ptr<NetRecord::SubnetRecord> pCurrentSubnetRecord;

            std::shared_ptr<PackageRecord> pCurrentPackageRecord;
            std::shared_ptr<PackageRecord::PinRecord> pCurrentPinRecord;

            std::shared_ptr<PackageRecord::OutlineRecord> pCurrentContourOutlineRecord;
            std::shared_ptr<ContourPolygon> pCurrentContourPolygon;

            FeatureGroupRecord::shared_ptr pCurrentFeatureGroupRecord;

            while (std::getline(edaDataFile, line))
            {
                // keep track of line number
                lineNumber++;

                // trim whitespace from beginning and end of line
                Utils::str_trim(line);
                if (!line.empty())
                {
                    if (m_logAllLineParsing)
                    {
                        parse_info pi(m_path, line, lineNumber);
                        logdebug(pi.toString("Parsing line..."));
                    }

                    std::stringstream lineStream(line);
                   
                    if (line.find(ATTRIBUTE_NAME_TOKEN) == 0 ||
                        line.find(std::string(COMMENT_TOKEN) + ATTRIBUTE_NAME_TOKEN) == 0)  // backward compatibility dictates allowing comment character in front of attribute value token
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
                    else if (line.find(ATTRIBUTE_VALUE_TOKEN) == 0 ||
                        line.find(std::string(COMMENT_TOKEN) + ATTRIBUTE_VALUE_TOKEN) == 0) // backward compatibility dictates allowing comment character in front of attribute value token
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
                    else if (line.find(COMMENT_TOKEN) == 0)
                    {
                        // comment line
                        // TODO: attribute lines can begin with a comment for backward compatbility
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
                    else if (line.find(HEADER_RECORD_TOKEN) == 0)
                    {
                        // component record line
                        std::string token;

                        lineStream >> token;
                        if (token != HEADER_RECORD_TOKEN)
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        // read the rest of the line as the source
                        if (!std::getline(lineStream, m_source))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        Utils::str_trim(m_source);
                    }
                    else if (line.find(LAYER_NAMES_RECORD_TOKEN) == 0)
                    {
                        // component record line
                        std::string token;

                        lineStream >> token;
                        if (token != LAYER_NAMES_RECORD_TOKEN)
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        while (lineStream >> token)
                        {
                            m_layerNames.push_back(token);
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

                        float f;
                        while (lineStream >> f)
                        {
                            pPropertyRecord->floatValues.push_back(f);
                        }

                        if (pCurrentNetRecord != nullptr)
                        {
                            pCurrentNetRecord->m_propertyRecords.push_back(pPropertyRecord);
                        }
                        else if (pCurrentPackageRecord != nullptr)
                        {
                            pCurrentPackageRecord->m_propertyRecords.push_back(pPropertyRecord);
                        }
                        else if (pCurrentFeatureGroupRecord != nullptr)
                        {
                            pCurrentFeatureGroupRecord->m_propertyRecords.push_back(pPropertyRecord);
                        }
                        else
                        {
                            // top-level PRP record
                            m_propertyRecords.push_back(pPropertyRecord);
                        }                        
                    }
                    else if (line.find(NET_RECORD_TOKEN) == 0)
                    {
                        // net record line
                        std::string token;

                        if (!(lineStream >> token))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (token != NET_RECORD_TOKEN)
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        // finish current (previous) net record
                        if (pCurrentNetRecord != nullptr)
                        {
                            // finish up (any) current subnet record
                            if (pCurrentSubnetRecord != nullptr)
                            {
                                pCurrentNetRecord->m_subnetRecords.push_back(pCurrentSubnetRecord);
                                pCurrentSubnetRecord.reset();
                            }

                            m_netRecords.push_back(pCurrentNetRecord);                            
                            //m_netRecordsByName[pCurrentNetRecord->name] = pCurrentNetRecord;
                            pCurrentNetRecord.reset();
                        }

                        // net name
                        if (!std::getline(lineStream, token, ';'))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        // trim whitespace from beginning                        
                        str_trim(token);

                        // create new net record
                        pCurrentNetRecord = std::make_shared<NetRecord>();
                        pCurrentNetRecord->name = token;
                        pCurrentNetRecord->index = static_cast<unsigned>(m_netRecords.size());

                        std::string attrIdString;
                        lineStream >> attrIdString;
                        if (!pCurrentNetRecord->ParseAttributeLookupTable(attrIdString))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }
                    }
                    else if (line.find(NetRecord::SubnetRecord::RECORD_TOKEN) == 0)
                    {
                        // component record line
                        std::string token;

                        lineStream >> token;
                        if (token != NetRecord::SubnetRecord::RECORD_TOKEN)
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (pCurrentNetRecord != nullptr)
                        {
                            // finish current (previous) subnet record
                            if (pCurrentSubnetRecord != nullptr)
                            {
                                pCurrentNetRecord->m_subnetRecords.push_back(pCurrentSubnetRecord);
                                pCurrentSubnetRecord.reset();
                            }
                        }

                        pCurrentSubnetRecord = std::make_shared<NetRecord::SubnetRecord>();
                        pCurrentSubnetRecord->index = static_cast<unsigned>(pCurrentNetRecord->m_subnetRecords.size());

                        // subnet type
                        lineStream >> token;
                        if (token == NetRecord::SubnetRecord::RECORD_TYPE_VIA_TOKEN)
                        {
                            pCurrentSubnetRecord->type = NetRecord::SubnetRecord::Type::Via;
                        }
                        else if (token == NetRecord::SubnetRecord::RECORD_TYPE_TRACE_TOKEN)
                        {
                            pCurrentSubnetRecord->type = NetRecord::SubnetRecord::Type::Trace;
                        }
                        else if (token == NetRecord::SubnetRecord::RECORD_TYPE_PLANE_TOKEN)
                        {
                            pCurrentSubnetRecord->type = NetRecord::SubnetRecord::Type::Plane;

                            // fill type
                            lineStream >> token;
                            if (token == "S")
                            {
                                pCurrentSubnetRecord->fillType = NetRecord::SubnetRecord::FillType::Solid;
                            }
                            else if (token == "O")
                            {
                                pCurrentSubnetRecord->fillType = NetRecord::SubnetRecord::FillType::Outline;
                            }
                            else
                            {
                                throw_parse_error(m_path, line, token, lineNumber);
                            }

                            // cutout type
                            lineStream >> token;
                            if (token == "C")
                            {
                                pCurrentSubnetRecord->cutoutType = NetRecord::SubnetRecord::CutoutType::Circle;
                            }
                            else if (token == "R")
                            {
                                pCurrentSubnetRecord->cutoutType = NetRecord::SubnetRecord::CutoutType::Rectangle;
                            }
                            else if (token == "O")
                            {
                                pCurrentSubnetRecord->cutoutType = NetRecord::SubnetRecord::CutoutType::Octagon;
                            }
                            else if (token == "E")
                            {
                                pCurrentSubnetRecord->cutoutType = NetRecord::SubnetRecord::CutoutType::Exact;
                            }
                            else
                            {
                                throw_parse_error(m_path, line, token, lineNumber);
                            }

                            // fill size
                            lineStream >> pCurrentSubnetRecord->fillSize;
                        }
                        else if (token == NetRecord::SubnetRecord::RECORD_TYPE_TOEPRINT_TOKEN)
                        {
                            pCurrentSubnetRecord->type = NetRecord::SubnetRecord::Type::Toeprint;

                            // side
                            lineStream >> token;
                            if (token == "T")
                            {
                                pCurrentSubnetRecord->side = BoardSide::Top;
                            }
                            else if (token == "B")
                            {
                                pCurrentSubnetRecord->side = BoardSide::Bottom;
                            }
                            else
                            {
                                throw_parse_error(m_path, line, token, lineNumber);
                            }

                            // componentNumber
                            lineStream >> pCurrentSubnetRecord->componentNumber;

                            // toeprintNumber
                            lineStream >> pCurrentSubnetRecord->toeprintNumber;
                        }
                        else
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }
                    }
                    else if (line.find(FEATURE_ID_RECORD_TOKEN) == 0)
                    {
                        // component record line
                        std::string token;

                        if (!(lineStream >> token) || token != FEATURE_ID_RECORD_TOKEN)
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        auto pFeatureIdRecord = std::make_shared<FeatureIdRecord>();

                        // type
                        if (!(lineStream >> token))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (token == "C")
                        {
                            pFeatureIdRecord->type = FeatureIdRecord::Type::Copper;
                        }
                        else if (token == "L")
                        {
                            pFeatureIdRecord->type = FeatureIdRecord::Type::Laminate;
                        }
                        else if (token == "H")
                        {
                            pFeatureIdRecord->type = FeatureIdRecord::Type::Hole;
                        }
                        else
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        // layer number
                        if (!(lineStream >> pFeatureIdRecord->layerNumber))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        // feature number
                        if (!(lineStream >> pFeatureIdRecord->featureNumber))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (pCurrentNetRecord != nullptr &&
                            pCurrentSubnetRecord != nullptr)
                        {
                            pCurrentSubnetRecord->m_featureIdRecords.push_back(pFeatureIdRecord);
                        }
                        else if (pCurrentFeatureGroupRecord != nullptr)
                        {
                            pCurrentFeatureGroupRecord->m_featureIdRecords.push_back(pFeatureIdRecord);
                        }
                        else
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }
                    }
                    else if (line.find(PACKAGE_RECORD_TOKEN) == 0)
                    {
                        // package record line
                        std::string token;

                        if (!(lineStream >> token) || token != PACKAGE_RECORD_TOKEN)
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        // finish current (previous) net record
                        if (pCurrentNetRecord != nullptr)
                        {
                            // finish up (any) current subnet record
                            if (pCurrentSubnetRecord != nullptr)
                            {
                                pCurrentNetRecord->m_subnetRecords.push_back(pCurrentSubnetRecord);
                                pCurrentSubnetRecord.reset();
                            }

                            m_netRecords.push_back(pCurrentNetRecord);
                            //m_netRecordsByName[pCurrentNetRecord->name] = pCurrentNetRecord;
                            pCurrentNetRecord.reset();
                        }
                        else if (pCurrentPackageRecord != nullptr)
                        {
                            // finish up any current (previous) pin records
                            if (pCurrentPinRecord != nullptr)
                            {
                                // check for overflow
                                if (pCurrentPackageRecord->m_pinRecords.size() > UINT_MAX)
								{
									throw_parse_error(m_path, line, token, lineNumber);
								}
                                pCurrentPinRecord->index = static_cast<unsigned>(pCurrentPackageRecord->m_pinRecords.size());
                                pCurrentPackageRecord->m_pinRecords.push_back(pCurrentPinRecord);
                                pCurrentPinRecord.reset();
                            }

                            // finish up any current (previous) package records
                            m_packageRecords.push_back(pCurrentPackageRecord);
                            //m_packageRecordsByName[pCurrentPackageRecord->name] = pCurrentPackageRecord;
                            pCurrentPackageRecord.reset();
                        }

                        pCurrentPackageRecord = std::make_shared<PackageRecord>();
                        pCurrentPackageRecord->index = static_cast<unsigned int>(m_packageRecords.size());

                        // name
                        if (!(lineStream >> pCurrentPackageRecord->name))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        // pitch
                        if (!(lineStream >> pCurrentPackageRecord->pitch))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        // xmin, ymin
                        if (!(lineStream >> pCurrentPackageRecord->xMin >> pCurrentPackageRecord->yMin))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        // xmax
                        if (!(lineStream >> pCurrentPackageRecord->xMax))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        // ymax and attributes/ID string
                        if (!std::getline(lineStream, token, ';'))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        // ymax
                        pCurrentPackageRecord->yMax = std::stof(token);

                        std::string attrIdString;
                        lineStream >> attrIdString;
                        if (!pCurrentPackageRecord->ParseAttributeLookupTable(attrIdString))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }
                    }
                    else if (line.find(PIN_RECORD_TOKEN) == 0)
                    {
                        // package pin record line
                        std::string token;

                        if (!(lineStream >> token) || token != PIN_RECORD_TOKEN)
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (pCurrentPackageRecord != nullptr)
                        {
                            // finish up any current (previous) pin records
                            if (pCurrentPinRecord != nullptr)
                            {
                                // check for overflow
                                if (pCurrentPackageRecord->m_pinRecords.size() > UINT_MAX)
                                {
                                    throw_parse_error(m_path, line, token, lineNumber);
                                }
                                pCurrentPinRecord->index = static_cast<unsigned>(pCurrentPackageRecord->m_pinRecords.size());
                                pCurrentPackageRecord->m_pinRecords.push_back(pCurrentPinRecord);
                                pCurrentPinRecord.reset();
                            }                            
                        }

                        pCurrentPinRecord = std::make_shared<PackageRecord::PinRecord>();
                        //pCurrentPinRecord->index = static_cast<unsigned>(pCurrentPackageRecord->m_pinRecords.size());

                        // name
                        lineStream >> pCurrentPinRecord->name;

                        // type
                        if (!(lineStream >> token))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (token == "T")
                        {
                            pCurrentPinRecord->type = PackageRecord::PinRecord::Type::ThroughHole;
                        }
                        else if (token == "B")
                        {
                            pCurrentPinRecord->type = PackageRecord::PinRecord::Type::Blind;
                        }
                        else if (token == "S")
                        {
                            pCurrentPinRecord->type = PackageRecord::PinRecord::Type::Surface;
                        }
                        else
                        {
                            logerror("unknown pin type: [" + token + "]");
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        // xc, xy
                        if (!(lineStream >> pCurrentPinRecord->xCenter >> pCurrentPinRecord->yCenter))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        // finished hole size (fhs)
                        if (!(lineStream >> pCurrentPinRecord->finishedHoleSize))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        // electrical type
                        if (!(lineStream >> token))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (token == "E")
                        {
                            pCurrentPinRecord->electricalType = PackageRecord::PinRecord::ElectricalType::Electrical;
                        }
                        else if (token == "M")
                        {
                            pCurrentPinRecord->electricalType = PackageRecord::PinRecord::ElectricalType::NonElectrical;
                        }
                        else if (token == "U")
                        {
                            pCurrentPinRecord->electricalType = PackageRecord::PinRecord::ElectricalType::Undefined;
                        }
                        else
                        {
                            logerror("unknown pin electrical type: [" + token + "]");
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        // mount type
                        if (!(lineStream >> token))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (token == "S")
                        {
                            pCurrentPinRecord->mountType = PackageRecord::PinRecord::MountType::Smt;
                        }
                        else if (token == "D")
                        {
                            pCurrentPinRecord->mountType = PackageRecord::PinRecord::MountType::RecommendedSmtPad;
                        }
                        else if (token == "T")
                        {
                            pCurrentPinRecord->mountType = PackageRecord::PinRecord::MountType::MT_ThroughHole;
                        }
                        else if (token == "R")
                        {
                            pCurrentPinRecord->mountType = PackageRecord::PinRecord::MountType::RecommendedThroughHole;
                        }
                        else if (token == "P")
                        {
                            pCurrentPinRecord->mountType = PackageRecord::PinRecord::MountType::Pressfit;
                        }
                        else if (token == "N")
                        {
                            pCurrentPinRecord->mountType = PackageRecord::PinRecord::MountType::NonBoard;
                        }
                        else if (token == "H")
                        {
                            pCurrentPinRecord->mountType = PackageRecord::PinRecord::MountType::Hole;
                        }
                        else if (token == "U")
                        {
                            pCurrentPinRecord->mountType = PackageRecord::PinRecord::MountType::MT_Undefined;
                        }
                        else
                        {
                            logerror("unknown pin mount type: [" + token + "]");
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        // ID=<id>
                        // PIN record, ID field is optional: spec pg.31
                        if (lineStream >> token)
                        {
                            std::stringstream idStream(token);
                            if (!std::getline(idStream, token, '=') || token != "ID")
                            {
                                throw_parse_error(m_path, line, token, lineNumber);
                            }

                            idStream >> pCurrentPinRecord->id;
                        }
                        else
                        {

#define SIMULATE_PARSE_ERROR 0
#if SIMULATE_PARSE_ERROR
                            throw_parse_error(m_path, line, token, lineNumber);
#endif

                            pCurrentPinRecord->id = UINT_MAX;
                        }                        
                    }
                    else if (line.find(FEATURE_GROUP_RECORD_TOKEN) == 0)
                    {
                        // feature group record line
                        std::string token;

                        if (!(lineStream >> token))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }
                        
                        if (token != FEATURE_GROUP_RECORD_TOKEN)
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        // finish current (previous) net record
                        if (pCurrentNetRecord != nullptr)
                        {
                            // finish up (any) current subnet record
                            if (pCurrentSubnetRecord != nullptr)
                            {
                                pCurrentNetRecord->m_subnetRecords.push_back(pCurrentSubnetRecord);
                                pCurrentSubnetRecord.reset();
                            }

                            m_netRecords.push_back(pCurrentNetRecord);
                            //m_netRecordsByName[pCurrentNetRecord->name] = pCurrentNetRecord;
                            pCurrentNetRecord.reset();
                        }
                        else if (pCurrentPackageRecord != nullptr)
                        {
                            // finish up any current (previous) pin records
                            if (pCurrentPinRecord != nullptr)
                            {
                                // check for overflow
                                if (pCurrentPackageRecord->m_pinRecords.size() > UINT_MAX)
                                {
                                    throw_parse_error(m_path, line, token, lineNumber);
                                }
                                pCurrentPinRecord->index = static_cast<unsigned>(pCurrentPackageRecord->m_pinRecords.size());
                                pCurrentPackageRecord->m_pinRecords.push_back(pCurrentPinRecord);
                                pCurrentPinRecord.reset();
                            }

                            // finish up any current (previous) package records
                            m_packageRecords.push_back(pCurrentPackageRecord);
                            //m_packageRecordsByName[pCurrentPackageRecord->name] = pCurrentPackageRecord;
                            pCurrentPackageRecord.reset();
                        }                        
                        else if (pCurrentFeatureGroupRecord != nullptr)
                        {
                            m_featureGroupRecords.push_back(pCurrentFeatureGroupRecord);
                            pCurrentFeatureGroupRecord.reset();
                        }

                        pCurrentFeatureGroupRecord = std::make_shared<FeatureGroupRecord>();

                        if (!(lineStream >> pCurrentFeatureGroupRecord->type))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }                                                
                    }
                    else if (line.find(PackageRecord::OutlineRecord::RECTANGLE_RECORD_TOKEN) == 0)
                    {                        
                        std::string token;                        
                        if (!(lineStream >> token))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (token != PackageRecord::OutlineRecord::RECTANGLE_RECORD_TOKEN)
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        auto pOutlineRecord = std::make_shared<PackageRecord::OutlineRecord>();
                        pOutlineRecord->type = PackageRecord::OutlineRecord::Type::Rectangle;

                        if (!(lineStream >> pOutlineRecord->lowerLeftX))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (!(lineStream >> pOutlineRecord->lowerLeftY))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (!(lineStream >> pOutlineRecord->width))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (!(lineStream >> pOutlineRecord->height))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (pCurrentPackageRecord != nullptr)
                        {
                            if (pCurrentPinRecord != nullptr)
                            {
                                pCurrentPinRecord->m_outlineRecords.push_back(pOutlineRecord);
                            }
                            else
                            {
                                pCurrentPackageRecord->m_outlineRecords.push_back(pOutlineRecord);
                            }
                        }
                        else
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }
                    }
                    else if (line.find(PackageRecord::OutlineRecord::CIRCLE_RECORD_TOKEN) == 0)
                    {
                        std::string token;
                        if (!(lineStream >> token))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (token != PackageRecord::OutlineRecord::CIRCLE_RECORD_TOKEN)
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        auto pOutlineRecord = std::make_shared<PackageRecord::OutlineRecord>();
                        pOutlineRecord->type = PackageRecord::OutlineRecord::Type::Circle;

                        if (!(lineStream >> pOutlineRecord->xCenter))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (!(lineStream >> pOutlineRecord->yCenter))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (!(lineStream >> pOutlineRecord->radius))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }                        

                        if (pCurrentPackageRecord != nullptr)
                        {
                            if (pCurrentPinRecord != nullptr)
                            {
                                pCurrentPinRecord->m_outlineRecords.push_back(pOutlineRecord);
                            }
                            else
                            {
                                pCurrentPackageRecord->m_outlineRecords.push_back(pOutlineRecord);
                            }
                        }
                        else
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }
                    }
                    else if (line.find(PackageRecord::OutlineRecord::SQUARE_RECORD_TOKEN) == 0)
                    {
                        std::string token;
                        if (!(lineStream >> token))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (token != PackageRecord::OutlineRecord::SQUARE_RECORD_TOKEN)
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        auto pOutlineRecord = std::make_shared<PackageRecord::OutlineRecord>();
                        pOutlineRecord->type = PackageRecord::OutlineRecord::Type::Square;

                        if (!(lineStream >> pOutlineRecord->xCenter))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (!(lineStream >> pOutlineRecord->yCenter))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (!(lineStream >> pOutlineRecord->halfSide))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (pCurrentPackageRecord != nullptr)
                        {
                            if (pCurrentPinRecord != nullptr)
                            {
                                pCurrentPinRecord->m_outlineRecords.push_back(pOutlineRecord);
                            }
                            else
                            {
                                pCurrentPackageRecord->m_outlineRecords.push_back(pOutlineRecord);
                            }
                        }
                        else
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }
                    }
                    else if (line.find(PackageRecord::OutlineRecord::CONTOUR_BEGIN_RECORD_TOKEN) == 0)
                    {
                        std::string token;
                        if (!(lineStream >> token))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (token != PackageRecord::OutlineRecord::CONTOUR_BEGIN_RECORD_TOKEN)
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        pCurrentContourOutlineRecord = std::make_shared<PackageRecord::OutlineRecord>();
                        pCurrentContourOutlineRecord->type = PackageRecord::OutlineRecord::Type::Contour;
                    }
					else if (line.find(PackageRecord::OutlineRecord::CONTOUR_END_RECORD_TOKEN) == 0)
					{
						std::string token;
						if (!(lineStream >> token))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (token != PackageRecord::OutlineRecord::CONTOUR_END_RECORD_TOKEN)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}                        

						if (pCurrentPackageRecord != nullptr)
						{
							if (pCurrentPinRecord != nullptr)
							{
								pCurrentPinRecord->m_outlineRecords.push_back(pCurrentContourOutlineRecord);
							}
							else
							{
								pCurrentPackageRecord->m_outlineRecords.push_back(pCurrentContourOutlineRecord);
							}
						}
						else
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						pCurrentContourOutlineRecord.reset();
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

                        if (pCurrentContourOutlineRecord != nullptr)
                        {
                            pCurrentContourOutlineRecord->m_contourPolygons.push_back(pCurrentContourPolygon);
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
                        logwarn(pi.toString("unrecognized record line in EDADATA file:"));                        
                    }
                }
                else
                {
                    // empty line
                    continue;                    
                }
            }

            // finish current (previous) net record
            // this is the case where the last line of the file is not a net record (and there are no PKG records)
            if (pCurrentNetRecord != nullptr)
            {
                // finish current (previous) subnet record
                // case where we reach the end of the file before a new net record is encountered (and there are no PKG records)
                if (pCurrentSubnetRecord != nullptr)
                {
                    pCurrentNetRecord->m_subnetRecords.push_back(pCurrentSubnetRecord);
                    pCurrentSubnetRecord.reset();
                }

                m_netRecords.push_back(pCurrentNetRecord);
                //m_netRecordsByName[pCurrentNetRecord->name] = pCurrentNetRecord;
                pCurrentNetRecord.reset();
            }
            else if (pCurrentPackageRecord != nullptr)
            {
                // finish up any current (previous) pin records
                if (pCurrentPinRecord != nullptr)
                {
                    // check for overflow
                    if (pCurrentPackageRecord->m_pinRecords.size() > UINT_MAX)
                    {
                        throw_parse_error(m_path, line, "", lineNumber);
                    }
                    pCurrentPinRecord->index = static_cast<unsigned>(pCurrentPackageRecord->m_pinRecords.size());
                    pCurrentPackageRecord->m_pinRecords.push_back(pCurrentPinRecord);
                    pCurrentPinRecord.reset();
                }

                // finish up any current (previous) package records
                m_packageRecords.push_back(pCurrentPackageRecord);
                //m_packageRecordsByName[pCurrentPackageRecord->name] = pCurrentPackageRecord;
                pCurrentPackageRecord.reset();
            }
            else if (pCurrentFeatureGroupRecord != nullptr)
            {
                m_featureGroupRecords.push_back(pCurrentFeatureGroupRecord);
                pCurrentFeatureGroupRecord.reset();
            }

            edaDataFile.close();
        }
        catch (parse_error& pe)
        {            
            auto m = pe.toString("Parse Error:");
            logerror(m);
            // cleanup file
            edaDataFile.close();
            throw pe;
        }
        catch (std::exception& e)
        {
            parse_info pi(m_path, line, lineNumber);
            const auto m = pi.toString();
            logexception_msg(e, m);
            // cleanup file
            edaDataFile.close();
            throw e;
        }

        return true;
    }       

    // Inherited via IProtoBuffable
    std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile::PackageRecord>
    EdaDataFile::PackageRecord::to_protobuf() const
    {                      
        std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile::PackageRecord> pPackageRecordMessage(new Odb::Lib::Protobuf::EdaDataFile::PackageRecord);
        pPackageRecordMessage->set_name(name);
        pPackageRecordMessage->set_pitch(pitch);
        pPackageRecordMessage->set_xmin(xMin);
        pPackageRecordMessage->set_ymin(yMin);
        pPackageRecordMessage->set_xmax(xMax);
        pPackageRecordMessage->set_ymax(yMax);
        //pPackageRecordMessage->set_attributesidstring(attributesIdString);
        for (const auto& pinRecord : m_pinRecords)
        {
            auto pPinRecordMessage = pPackageRecordMessage->add_pinrecords();
            pPinRecordMessage->CopyFrom(*pinRecord->to_protobuf());
        }
        for (const auto& kvPinRecord : m_pinRecordsByName)
        {
            (*pPackageRecordMessage->mutable_pinrecordsbyname())[kvPinRecord.first] = *kvPinRecord.second->to_protobuf();
        }
        for (const auto& propertyRecord : m_propertyRecords)
        {
            auto pPropertyRecordMessage = pPackageRecordMessage->add_propertyrecords();
            pPropertyRecordMessage->CopyFrom(*propertyRecord->to_protobuf());
        }
        for (const auto& outlineRecord : m_outlineRecords)
        {
			auto pOutlineRecordMessage = pPackageRecordMessage->add_outlinerecords();
			pOutlineRecordMessage->CopyFrom(*outlineRecord->to_protobuf());
		}
        for (const auto& kvAttributeAssignment : m_attributeLookupTable)
        {
            (*pPackageRecordMessage->mutable_attributelookuptable())[kvAttributeAssignment.first] = kvAttributeAssignment.second;
        }
        return pPackageRecordMessage;
    }

    void EdaDataFile::PackageRecord::from_protobuf(const Odb::Lib::Protobuf::EdaDataFile::PackageRecord& message)
    {
        name = message.name();
        pitch = message.pitch();
        xMin = message.xmin();
        yMin = message.ymin();
        xMax = message.xmax();
        yMax = message.ymax();
        //attributesIdString = message.attributesidstring();

        for (const auto& pinRecordMessage : message.pinrecords())
        {
			auto pPinRecord = std::make_shared<PinRecord>();
			pPinRecord->from_protobuf(pinRecordMessage);
			m_pinRecords.push_back(pPinRecord);			
		}

        for (const auto& kvPinRecordMessage : message.pinrecordsbyname())
        {
            auto pPinRecord = std::make_shared<PinRecord>();
            pPinRecord->from_protobuf(kvPinRecordMessage.second);
            m_pinRecordsByName[kvPinRecordMessage.first] = pPinRecord;
        }

        for (const auto& propertyRecordMessage : message.propertyrecords())
        {
			auto pPropertyRecord = std::make_shared<PropertyRecord>();
			pPropertyRecord->from_protobuf(propertyRecordMessage);
			m_propertyRecords.push_back(pPropertyRecord);
		}

        for (const auto& outlineRecordMessage : message.outlinerecords())
        {
            auto pOutlineRecord = std::make_shared<OutlineRecord>();
            pOutlineRecord->from_protobuf(outlineRecordMessage);
            m_outlineRecords.push_back(pOutlineRecord);
        }

        for (const auto& kvAttributeAssignment : message.attributelookuptable())
        {
            m_attributeLookupTable[kvAttributeAssignment.first] = kvAttributeAssignment.second;
        }
    }

    // Inherited via IProtoBuffable
    std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile::PackageRecord::OutlineRecord> EdaDataFile::PackageRecord::OutlineRecord::to_protobuf() const
    {
        std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile::PackageRecord::OutlineRecord> pOutlineRecordMessage(new Odb::Lib::Protobuf::EdaDataFile::PackageRecord::OutlineRecord);
		pOutlineRecordMessage->set_type((Odb::Lib::Protobuf::EdaDataFile::PackageRecord::OutlineRecord::Type)type);
		pOutlineRecordMessage->set_lowerleftx(lowerLeftX);
		pOutlineRecordMessage->set_lowerlefty(lowerLeftY);
		pOutlineRecordMessage->set_width(width);
		pOutlineRecordMessage->set_height(height);
		pOutlineRecordMessage->set_xcenter(xCenter);
		pOutlineRecordMessage->set_ycenter(yCenter);
		pOutlineRecordMessage->set_radius(radius);
		pOutlineRecordMessage->set_halfside(halfSide);
        for (const auto& contourPolygon : m_contourPolygons)
        {
			auto pContourPolygonMessage = pOutlineRecordMessage->add_contourpolygons();
			pContourPolygonMessage->CopyFrom(*contourPolygon->to_protobuf());
		}
		return pOutlineRecordMessage;
    }

    void EdaDataFile::PackageRecord::OutlineRecord::from_protobuf(const Odb::Lib::Protobuf::EdaDataFile::PackageRecord::OutlineRecord& message)
    {
        type = (Type)message.type();
		lowerLeftX = message.lowerleftx();
		lowerLeftY = message.lowerlefty();
		width = message.width();
		height = message.height();
		xCenter = message.xcenter();
		yCenter = message.ycenter();
		radius = message.radius();
		halfSide = message.halfside();
        for (const auto& contourPolygonMessage : message.contourpolygons())
        {
			auto pContourPolygon = std::make_shared<ContourPolygon>();
			pContourPolygon->from_protobuf(contourPolygonMessage);
			m_contourPolygons.push_back(pContourPolygon);
		}
    }

    // Inherited via IProtoBuffable
    std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile::PackageRecord::PinRecord>
    EdaDataFile::PackageRecord::PinRecord::to_protobuf() const
    {       
        std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile::PackageRecord::PinRecord> pPinRecordMessage(new Odb::Lib::Protobuf::EdaDataFile::PackageRecord::PinRecord);
        pPinRecordMessage->set_name(name);
        pPinRecordMessage->set_type((Odb::Lib::Protobuf::EdaDataFile::PackageRecord::PinRecord::Type)type);
        pPinRecordMessage->set_xcenter(xCenter);
        pPinRecordMessage->set_ycenter(yCenter);
        pPinRecordMessage->set_finishedholesize(finishedHoleSize);
        pPinRecordMessage->set_electricaltype((Odb::Lib::Protobuf::EdaDataFile::PackageRecord::PinRecord::ElectricalType)electricalType);
        pPinRecordMessage->set_mounttype((Odb::Lib::Protobuf::EdaDataFile::PackageRecord::PinRecord::MountType)mountType);
        pPinRecordMessage->set_id(id);
        pPinRecordMessage->set_index(index);
        return pPinRecordMessage;
    }

    void EdaDataFile::PackageRecord::PinRecord::from_protobuf(const Odb::Lib::Protobuf::EdaDataFile::PackageRecord::PinRecord& message)
    {
        name = message.name();
        type = (Type)message.type();
        xCenter = message.xcenter();
        yCenter = message.ycenter();
        finishedHoleSize = message.finishedholesize();
        electricalType = (ElectricalType)message.electricaltype();
        mountType = (MountType)message.mounttype();
        id = message.id();
        index = message.index();
    }
}