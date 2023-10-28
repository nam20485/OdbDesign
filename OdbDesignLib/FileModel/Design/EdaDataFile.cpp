#include "../../crow_win.h"
#include "EdaDataFile.h"
#include <fstream>
#include <sstream>
#include "str_trim.h"
#include "../../enums.h"
#include "../../ProtoBuf/edadatafile.pb.h"
#include "google/protobuf/message.h"


namespace Odb::Lib::FileModel::Design
{
    EdaDataFile::EdaDataFile()    
    {
    }
   
    EdaDataFile::~EdaDataFile()
    {
    }

    const std::filesystem::path& EdaDataFile::GetPath() const
    {
        return m_path;
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
            pSubnetRecordMessage->set_side((Odb::Lib::Protobuf::EdaDataFile_BoardSide)side);
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
        pNetRecordMessage->set_attributesidstring(attributesIdString);
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
        return pNetRecordMessage;
    }

    void EdaDataFile::NetRecord::from_protobuf(const Odb::Lib::Protobuf::EdaDataFile::NetRecord& message)
    {
      
    }

    std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile::NetRecord::SubnetRecord::FeatureIdRecord>
    EdaDataFile::NetRecord::SubnetRecord::FeatureIdRecord::to_protobuf() const
    {
        std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile::NetRecord::SubnetRecord::FeatureIdRecord> pFeatureIdRecordMessage(new Odb::Lib::Protobuf::EdaDataFile::NetRecord::SubnetRecord::FeatureIdRecord);        
        pFeatureIdRecordMessage->set_type((Odb::Lib::Protobuf::EdaDataFile::NetRecord::SubnetRecord::FeatureIdRecord::Type) type);
        pFeatureIdRecordMessage->set_layernumber(layerNumber);
        pFeatureIdRecordMessage->set_featurenumber(featureNumber);
        return pFeatureIdRecordMessage;
    }

    void EdaDataFile::NetRecord::SubnetRecord::FeatureIdRecord::from_protobuf(const Odb::Lib::Protobuf::EdaDataFile::NetRecord::SubnetRecord::FeatureIdRecord& message)
    {

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
        return pEdaDataFileMessage;
    }

    void EdaDataFile::from_protobuf(const Odb::Lib::Protobuf::EdaDataFile& message)
    {
       
    }    

    bool EdaDataFile::Parse(std::filesystem::path path)
    {
        m_path = path;

        auto edaDataFilePath = m_path / "data";

        if (!std::filesystem::exists(edaDataFilePath)) return false;
        else if (!std::filesystem::is_regular_file(edaDataFilePath)) return false;

        std::ifstream edaDataFile;
        edaDataFile.open(edaDataFilePath.string(), std::ios::in);
        if (!edaDataFile.is_open()) return false;

        std::shared_ptr<NetRecord> pCurrentNetRecord;
        std::shared_ptr<NetRecord::SubnetRecord> pCurrentSubnetRecord;

        std::shared_ptr<PackageRecord> pCurrentPackageRecord;
        std::shared_ptr<PackageRecord::PinRecord> pCurrentPinRecord;

        std::string line;
        while (std::getline(edaDataFile, line))
        {
            // trim whitespace from beginning and end of line
            Utils::str_trim(line);
            if (!line.empty())
            {
                std::stringstream lineStream(line);

                if (line.find(ATTRIBUTE_NAME_TOKEN) == 0 ||
                    line.find(COMMENT_TOKEN + ATTRIBUTE_NAME_TOKEN) == 0)  // backward compatibility dictates allowing comment character in front of attribute value token
                {
                    // component attribute name line	
                    std::string token;
                    // TODO: continue on failing line parse, to make a less strict/more robust parser (make a flag: enum ParseStrictness { strict, lax })
                    if (!std::getline(lineStream, token, ' ')) return false;
                    else if (!std::getline(lineStream, token, ' ')) return false;
                    m_attributeNames.push_back(token);
                }
                else if (line.find(ATTRIBUTE_VALUE_TOKEN) == 0 ||
                    line.find(COMMENT_TOKEN + ATTRIBUTE_VALUE_TOKEN) == 0) // backward compatibility dictates allowing comment character in front of attribute value token
                {
                    // component attribute text string values	
                    std::string token;
                    if (!std::getline(lineStream, token, ' ')) return false;
                    else if (!std::getline(lineStream, token, ' ')) return false;
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
                    if (!std::getline(lineStream, token, '=')) return false;
                    else if (!std::getline(lineStream, token, '=')) return false;
                    m_units = token;
                }
                else if (line.find(HEADER_RECORD_TOKEN) == 0)
                {
                    // component record line
                    std::string token;

                    lineStream >> token;
                    if (token != HEADER_RECORD_TOKEN) return false;

                    // read the rest of the line as the source
                    if (!std::getline(lineStream, m_source)) return false;
                    Utils::str_trim(m_source);
                }
                else if (line.find(LAYER_NAMES_RECORD_TOKEN) == 0)
                {
                    // component record line
                    std::string token;

                    lineStream >> token;
                    if (token != LAYER_NAMES_RECORD_TOKEN) return false;

                    while (lineStream >> token)
                    {
                        m_layerNames.push_back(token);
                    }
                }
                else if (line.find(PropertyRecord::RECORD_TOKEN) == 0)
                {
                    // component property record line
                    std::string token;
                    lineStream >> token;
                    if (token != PropertyRecord::RECORD_TOKEN) return false;

                    auto pPropertyRecord = std::make_shared<PropertyRecord>();
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

                    // TODO: add to current net OR package record
                    if (pCurrentNetRecord != nullptr)
                    {
                        pCurrentNetRecord->m_propertyRecords.push_back(pPropertyRecord);
                    }
                    else if (pCurrentPackageRecord != nullptr)
                    {
                        pCurrentPackageRecord->m_propertyRecords.push_back(pPropertyRecord);
                    }
                    else
                    {
                        // no current net or package record to put the property record in
                        return false;
                    }
                }
                else if (line.find(NET_RECORD_TOKEN) == 0)
                {
                    // net record line
                    std::string token;

                    lineStream >> token;
                    if (token != NET_RECORD_TOKEN) return false;

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
                        pCurrentNetRecord.reset();
                    }

                    // net name
                    if (!std::getline(lineStream, token, ';')) return false;

                    // create new net record
                    pCurrentNetRecord = std::make_shared<NetRecord>();
                    pCurrentNetRecord->name = token;

                    lineStream >> pCurrentNetRecord->attributesIdString;
                }
                else if (line.find(NetRecord::SubnetRecord::RECORD_TOKEN) == 0)
                {
                    // component record line
                    std::string token;

                    lineStream >> token;
                    if (token != NetRecord::SubnetRecord::RECORD_TOKEN) return false;

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
                            return false;
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
                            return false;
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
                            return false;
                        }

                        // componentNumber
                        lineStream >> pCurrentSubnetRecord->componentNumber;

                        // toeprintNumber
                        lineStream >> pCurrentSubnetRecord->toeprintNumber;
                    }
                    else
                    {
                        return false;
                    }
                }
                else if (line.find(FEATURE_ID_RECORD_TOKEN) == 0)
                {
                    // component record line
                    std::string token;

                    // TODO: second clause of if statement is redudant and should be removed
                    if (!(lineStream >> token) || token != FEATURE_ID_RECORD_TOKEN) return false;

                    auto pFeatureIdRecord = std::make_shared<NetRecord::SubnetRecord::FeatureIdRecord>();

                    // type
                    if (!(lineStream >> token)) return false;
                    if (token == "C")
                    {
                        pFeatureIdRecord->type = NetRecord::SubnetRecord::FeatureIdRecord::Type::Copper;
                    }
                    else if (token == "L")
                    {
                        pFeatureIdRecord->type = NetRecord::SubnetRecord::FeatureIdRecord::Type::Laminate;
                    }
                    else if (token == "H")
                    {
                        pFeatureIdRecord->type = NetRecord::SubnetRecord::FeatureIdRecord::Type::Hole;
                    }
                    else
                    {
                        return false;
                    }

                    // layer number
                    if (!(lineStream >> pFeatureIdRecord->layerNumber)) return false;

                    // feature number
                    if (!(lineStream >> pFeatureIdRecord->featureNumber)) return false;

                    if (pCurrentNetRecord != nullptr &&
                        pCurrentSubnetRecord != nullptr)
                    {
                        pCurrentSubnetRecord->m_featureIdRecords.push_back(pFeatureIdRecord);
                    }
                    else
                    {
                        return false;
                    }
                }
                else if (line.find(PACKAGE_RECORD_TOKEN) == 0)
                {
                    // package record line
                    std::string token;

                    if (!(lineStream >> token) || token != PACKAGE_RECORD_TOKEN) return false;

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
                        pCurrentNetRecord.reset();
                    }

                    if (pCurrentPackageRecord != nullptr)
                    {
                        // finish up any current (previous) pin records
                        if (pCurrentPinRecord != nullptr)
                        {
                            pCurrentPackageRecord->m_pinRecords.push_back(pCurrentPinRecord);
                            pCurrentPinRecord.reset();
                        }

                        // finish up any current (previous) package records
                        m_packageRecords.push_back(pCurrentPackageRecord);
                        pCurrentPackageRecord.reset();
                    }

                    pCurrentPackageRecord = std::make_shared<PackageRecord>();

                    // name
                    if (!(lineStream >> pCurrentPackageRecord->name)) return false;

                    // pitch
                    if (!(lineStream >> pCurrentPackageRecord->pitch)) return false;

                    // xmin, ymin
                    if (!(lineStream >> pCurrentPackageRecord->xMin >> pCurrentPackageRecord->yMin)) return false;

                    // xmax
                    if (!(lineStream >> pCurrentPackageRecord->xMax)) return false;

                    // ymax and attributes/ID string
                    if (!std::getline(lineStream, token, ';')) return false;

                    // ymax
                    pCurrentPackageRecord->yMax = std::stof(token);

                    lineStream >> pCurrentPackageRecord->attributesIdString;
                }
                else if (line.find(PIN_RECORD_TOKEN) == 0)
                {
                    // package pin record line
                    std::string token;

                    if (!(lineStream >> token) || token != PIN_RECORD_TOKEN) return false;

                    if (pCurrentPackageRecord != nullptr)
                    {
                        // finish up any current (previous) pin records
                        if (pCurrentPinRecord != nullptr)
                        {
                            pCurrentPackageRecord->m_pinRecords.push_back(pCurrentPinRecord);
                            pCurrentPinRecord.reset();
                        }
                    }

                    auto pPinRecord = std::make_shared<PackageRecord::PinRecord>();

                    // name
                    lineStream >> pPinRecord->name;

                    // type
                    if (!(lineStream >> token)) return false;
                    if (token == "T")
                    {
                        pPinRecord->type = PackageRecord::PinRecord::Type::ThroughHole;
                    }
                    else if (token == "B")
                    {
                        pPinRecord->type = PackageRecord::PinRecord::Type::Blind;
                    }
                    else if (token == "S")
                    {
                        pPinRecord->type = PackageRecord::PinRecord::Type::Surface;
                    }
                    else
                    {
                        return false;
                    }

                    // xc, xy
                    if (!(lineStream >> pPinRecord->xCenter >> pPinRecord->yCenter)) return false;

                    // finished hole size (fhs)
                    if (!(lineStream >> pPinRecord->finishedHoleSize)) return false;

                    // electrical type
                    if (!(lineStream >> token)) return false;
                    if (token == "E")
                    {
                        pPinRecord->electricalType = PackageRecord::PinRecord::ElectricalType::Electrical;
                    }
                    else if (token == "M")
                    {
                        pPinRecord->electricalType = PackageRecord::PinRecord::ElectricalType::NonElectrical;
                    }
                    else if (token == "U")
                    {
                        pPinRecord->electricalType = PackageRecord::PinRecord::ElectricalType::Undefined;
                    }
                    else
                    {
                        return false;
                    }

                    // mount type
                    if (!(lineStream >> token)) return false;
                    if (token == "S")
                    {
                        pPinRecord->mountType = PackageRecord::PinRecord::MountType::Smt;
                    }
                    else if (token == "D")
                    {
                        pPinRecord->mountType = PackageRecord::PinRecord::MountType::RecommendedSmtPad;
                    }
                    else if (token == "T")
                    {
                        pPinRecord->mountType = PackageRecord::PinRecord::MountType::MT_ThroughHole;
                    }
                    else if (token == "R")
                    {
                        pPinRecord->mountType = PackageRecord::PinRecord::MountType::RecommendedThroughHole;
                    }
                    else if (token == "P")
                    {
                        pPinRecord->mountType = PackageRecord::PinRecord::MountType::Pressfit;
                    }
                    else if (token == "N")
                    {
                        pPinRecord->mountType = PackageRecord::PinRecord::MountType::NonBoard;
                    }
                    else if (token == "H")
                    {
                        pPinRecord->mountType = PackageRecord::PinRecord::MountType::Hole;
                    }
                    else if (token == "U")
                    {
                        pPinRecord->mountType = PackageRecord::PinRecord::MountType::MT_Undefined;
                    }
                    else
                    {
                        return false;
                    }

                    // ID=<id>
                    if (!(lineStream >> token)) return false;
                    std::stringstream idStream(token);
                    if (!std::getline(idStream, token, '=') || token != "ID") return false;
                    idStream >> pPinRecord->id;

                    if (pCurrentPackageRecord != nullptr)
                    {                        
                        pPinRecord->index = pCurrentPackageRecord->m_pinRecords.size();
                        pCurrentPackageRecord->m_pinRecords.push_back(pPinRecord);
                    }
                    else
                    {
                        return false;
                    }
                }
                else if (line.find(FEATURE_GROUP_RECORD_TOKEN) == 0)
                {
                    // feature group record line
                    std::string token;

                    lineStream >> token;
                    if (token != FEATURE_GROUP_RECORD_TOKEN) return false;

                    // TODO: parse FGR records
                }
            }
            else
            {
                continue;
                //return false;
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
            pCurrentNetRecord.reset();
        }

        // TODO: ^^^ should be else if, i.e. there should only be one of either a
        // current net OR package record to close up when we reach the EOF, but not both
        if (pCurrentPackageRecord != nullptr)
        {
            // finish up any current (previous) pin records
            if (pCurrentPinRecord != nullptr)
            {
                pCurrentPackageRecord->m_pinRecords.push_back(pCurrentPinRecord);
                pCurrentPinRecord.reset();
            }

            // finish up any current (previous) package records
            m_packageRecords.push_back(pCurrentPackageRecord);
            pCurrentPackageRecord.reset();
        }

        return true;
    }

    // Inherited via IProtoBuffable
    std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile::PropertyRecord>
    EdaDataFile::PropertyRecord::to_protobuf() const
    {     
        std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile::PropertyRecord> pPropertyRecordMessage(new Odb::Lib::Protobuf::EdaDataFile::PropertyRecord);
        pPropertyRecordMessage->set_name(name);
        pPropertyRecordMessage->set_value(value);
        for (const auto& f : floatValues)
        {
			pPropertyRecordMessage->add_floatvalues(f);
		}
        return pPropertyRecordMessage;
    }

    void EdaDataFile::PropertyRecord::from_protobuf(const Odb::Lib::Protobuf::EdaDataFile::PropertyRecord& message)
    {
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
        pPackageRecordMessage->set_attributesidstring(attributesIdString);
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
        return pPackageRecordMessage;
    }

    void EdaDataFile::PackageRecord::from_protobuf(const Odb::Lib::Protobuf::EdaDataFile::PackageRecord& message)
    {
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
    }
}