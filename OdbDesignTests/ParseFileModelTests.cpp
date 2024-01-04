#include <gtest/gtest.h>
#include "Fixtures/FileArchiveLoadFixture.h"
#include "OdbDesign.h"
#include <macros.h>

//using namespace Odb::Lib::App;
using namespace Odb::Lib::FileModel;
using namespace Odb::Test::Fixtures;


bool TestRigidFlexDesign()
{
    std::string rigidFlexDesignPath;
    if (Odb::Lib::IsMsvc())
    {
        rigidFlexDesignPath = R"(C:\Users\nmill\OneDrive\Documents\ODB++\Samples\designodb_rigidflex.tgz)";
    }
    else
    {
        rigidFlexDesignPath = R"(/mnt/c/Users/nmill/OneDrive/Documents/ODB++/Samples/designodb_rigidflex.tgz)";
    }

    std::cout << "Processing " << rigidFlexDesignPath << "... " << std::endl;

    Odb::Lib::FileModel::Design::FileArchive rigidFlexOdbDesign(rigidFlexDesignPath);
    auto success = rigidFlexOdbDesign.ParseFileModel();
    if (!success)
    {
        return false;
    }

    const auto& findStep = rigidFlexOdbDesign.GetStepsByName().find("cellular_flip-phone");
    if (findStep != rigidFlexOdbDesign.GetStepsByName().end())
    {
        // step
        auto& pStep = findStep->second;
        auto name = pStep->GetName();

        // eda data
        auto& edaData = pStep->GetEdaDataFile();
        auto& netRecords = edaData.GetNetRecords();
        if (netRecords.size() > 20)
        {
            auto& pNetRecord = netRecords[20];
            if (pNetRecord->m_subnetRecords.size() > 44)
            {
                auto& pSubnetRecord = pNetRecord->m_subnetRecords[44];
                auto subnetType = pSubnetRecord->type;
                if (subnetType == Odb::Lib::FileModel::Design::EdaDataFile::NetRecord::SubnetRecord::Type::Toeprint)
                {
                    //auto pToeprintSubnetRecord = pSubnetRecord;
                    //auto viaType = pToeprintSubnetRecord->type;
                    //if (viaType == Odb::Lib::FileModel::Design::EdaDataFile::NetRecord::ToeprintSubnetRecord::Type::Via)
                    //{

                    //}
                }
            }
        }

        auto& packageRecords = edaData.GetPackageRecords();
        if (packageRecords.size() > 0)
        {
            auto& pPackageRecord = packageRecords[0];
            auto packageName = pPackageRecord->name;
        }

        // layers
        auto& layersByName = pStep->GetLayersByName();
        auto layerFind = layersByName.find(Odb::Lib::FileModel::Design::ComponentsFile::TOP_COMPONENTS_LAYER_NAME);
        if (layerFind != layersByName.end())
        {
            auto& pLayer = layerFind->second;
            auto layerName = pLayer->GetName();
        }

        // netlist
        const auto& netlistsByName = pStep->GetNetlistsByName();
        auto netlistFind = netlistsByName.find("cadnet");
        if (netlistFind != netlistsByName.end())
        {
            auto& pNetlist = netlistFind->second;
            auto netlistName = pNetlist->GetName();
            auto& netListNetRecords = pNetlist->GetNetRecords();
            for (auto& netRecord : netListNetRecords)
            {
                if (netRecord->netName == "")
                {

                }
            }
        }
    }

    return true;
}