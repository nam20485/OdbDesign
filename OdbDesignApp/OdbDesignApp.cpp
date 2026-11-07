// OdbDesignApp.cpp : Defines the entry point for the application.
//
#include "OdbDesignApp.h"
#include "OdbDesign.h"
#include "macros.h"
#include "ExitCode.h"
#include <iostream>
#include <cstdlib>
#include <filesystem>
#include <FileModel/Design/LayerDirectory.h>
#include <FileModel/Design/EdaDataFile.h>


bool TestRigidFlexDesign();
bool TestSampleDesign();


int main()
{
    std::cout << "OdbDesignApp v0.1.0" << std::endl << std::endl;

    //OdbDesign::Lib::helloLib();

    auto success = TestRigidFlexDesign();
    if (!success)
    {
        return (int) Utils::ExitCode::UnknownError;
    }

    //std::cout << "success!" << std::endl;

    success = TestSampleDesign();
    if (!success)
    {
        return (int) Utils::ExitCode::UnknownError;
    }

    //std::cout << "success!" << std::endl;    

    return (int) Utils::ExitCode::Success;
}

// Resolve a test design archive by filename from the ODB_TEST_DATA_DIR
// directory (same convention as the test suite). Returns an empty path when
// the environment variable is not set.
static std::string getTestDataPath(const std::string& filename)
{
    const char* testDataDir = std::getenv("ODB_TEST_DATA_DIR");
    if (testDataDir == nullptr || testDataDir[0] == '\0')
    {
        std::cout << "ODB_TEST_DATA_DIR is not set; cannot resolve '" << filename << "'" << std::endl;
        return {};
    }
    return (std::filesystem::path(testDataDir) / filename).string();
}

bool TestSampleDesign()
{
    auto sampleDesignPath = getTestDataPath("sample_design.tgz");
    if (sampleDesignPath.empty())
    {
        return false;
    }

    std::cout << "Processing " << sampleDesignPath << "... " << std::endl;

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
    auto rigidFlexDesignPath = getTestDataPath("designodb_rigidflex.tgz");
    if (rigidFlexDesignPath.empty())
    {
        return false;
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
            // auto& pNetlist = netlistFind->second;
            // auto netlistName = pNetlist->GetName();
            // auto& netListNetRecords = pNetlist->GetNetRecords();
			// for (auto& pNetRecord : netListNetRecords)
			// {
				
			// }
        }
    }

    return true;
}
