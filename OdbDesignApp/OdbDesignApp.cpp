// OdbDesignApp.cpp : Defines the entry point for the application.
//
#include "OdbDesignApp.h"
#include "FileArchive.h"
#include "macros.h"


bool TestRigidFlexDesign();
bool TestSampleDesign();


int main()
{
    std::cout << "OdbDesignApp v0.1.0" << std::endl << std::endl;

    //OdbDesign::Lib::helloLib();

    auto success = TestRigidFlexDesign();
    if (!success)
    {
        return 1;
    }

    std::cout << "success!" << std::endl;

    success = TestSampleDesign();
    if (!success)
    {
        return 1;
    }

    std::cout << "success!" << std::endl;    

    return 0;
}

bool TestSampleDesign()
{
    std::string sampleDesignPath;
    if (Odb::Lib::IsMsvc())
    {
        sampleDesignPath = R"(C:\Users\nmill\OneDrive\Documents\ODB++\Samples\sample_design)";
    }
    else
    {
        sampleDesignPath = R"(/mnt/c/Users/nmill/OneDrive/Documents/ODB++/Samples/sample_design)";
    }

    std::cout << "Parsing " << sampleDesignPath << "... ";

    Odb::Lib::FileModel::Design::FileArchive sampleOdbDesign(sampleDesignPath);
    auto success = sampleOdbDesign.ParseFileModel();
    if (! success)
    {     
        return false;
    }

    return true;
}

bool TestRigidFlexDesign()
{   
    std::string rigidFlexDesignPath;
    if (Odb::Lib::IsMsvc())
    {
        rigidFlexDesignPath = R"(C:\Users\nmill\OneDrive\Documents\ODB++\Samples\designodb_rigidflex)";
    }
    else
    {
        rigidFlexDesignPath = R"(/mnt/c/Users/nmill/OneDrive/Documents/ODB++/Samples/designodb_rigidflex)";
    }

    std::cout << "Parsing " << rigidFlexDesignPath << "... ";

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
                    auto pViaSubnetRecord = std::dynamic_pointer_cast<Odb::Lib::FileModel::Design::EdaDataFile::NetRecord::ToeprintSubnetRecord>(pSubnetRecord);
                    auto viaType = pViaSubnetRecord->type;
                    if (viaType == Odb::Lib::FileModel::Design::EdaDataFile::NetRecord::ToeprintSubnetRecord::Type::Via)
                    {

                    }
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
        auto layerFind = layersByName.find(Odb::Lib::FileModel::Design::LayerDirectory::TOP_COMPONENTS_LAYER_NAME);
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
            auto& netNames = pNetlist->GetNetNames();
            for (auto& netName : netNames)
            {
                if (netName == "")
                {

                }
            }
        }
    }

    return true;
}
