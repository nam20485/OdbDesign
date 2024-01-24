#include <gtest/gtest.h>
#include "Fixtures/FileArchiveLoadFixture.h"
#include "OdbDesign.h"

//using namespace Odb::Lib::App;
using namespace Odb::Lib::FileModel;
using namespace Odb::Test::Fixtures;

namespace Odb::Test
{
	TEST_F(FileArchiveLoadFixture, Test_DesignOdb_RigidFlexDesign_Can_Has_Data)
	{
        auto rigidFlexDesignPath = getDesignPath("designodb_rigidflex.tgz");

        Odb::Lib::FileModel::Design::FileArchive rigidFlexOdbDesign(rigidFlexDesignPath.string());
        auto success = rigidFlexOdbDesign.ParseFileModel();
        ASSERT_TRUE(success);

        const auto& findStep = rigidFlexOdbDesign.GetStepsByName().find("cellular_flip-phone");
        ASSERT_NE(findStep, rigidFlexOdbDesign.GetStepsByName().end());
       
        // step
        auto& pStep = findStep->second;
        auto name = pStep->GetName();
        ASSERT_STREQ(name.c_str(), "cellular_flip-phone");

        // eda data
        auto& edaData = pStep->GetEdaDataFile();
        auto& netRecords = edaData.GetNetRecords();
        ASSERT_GT(netRecords.size(), 20);

        auto& pNetRecord = netRecords[20];
        ASSERT_GT(pNetRecord->m_subnetRecords.size(), 44);
        auto& pSubnetRecord = pNetRecord->m_subnetRecords[44];        
        
        auto subnetType = pSubnetRecord->type;
        ASSERT_EQ(subnetType, Odb::Lib::FileModel::Design::EdaDataFile::NetRecord::SubnetRecord::Type::Toeprint);
        //auto pToeprintSubnetRecord = pSubnetRecord;
        //auto viaType = pToeprintSubnetRecord->type;
        //if (viaType == Odb::Lib::FileModel::Design::EdaDataFile::NetRecord::ToeprintSubnetRecord::Type::Via)
        //{
        //}        
        
        auto& packageRecords = edaData.GetPackageRecords();
        ASSERT_GT(packageRecords.size(), 0);
        auto& pPackageRecord = packageRecords[0];
        const auto& packageName = pPackageRecord->name;
        // assert package name

        // layers
        auto& layersByName = pStep->GetLayersByName();
        ASSERT_GT(layersByName.size(), 0);

        auto layerFind = layersByName.find(Odb::Lib::FileModel::Design::ComponentsFile::TOP_COMPONENTS_LAYER_NAME);
        ASSERT_NE(layerFind, layersByName.end());
        auto& pLayer = layerFind->second;
        ASSERT_NE(pLayer, nullptr);
        auto layerName = pLayer->GetName();    
        // assert layer name

        // netlist
        const auto& netlistsByName = pStep->GetNetlistsByName();
        ASSERT_GT(netlistsByName.size(), 0);

        auto netlistFind = netlistsByName.find("cadnet");
        ASSERT_NE(netlistFind, netlistsByName.end());
        auto& pNetlist = netlistFind->second;
        ASSERT_NE(pNetlist, nullptr);

        auto netlistName = pNetlist->GetName();
        // assert netlist name

        auto& netListNetRecords = pNetlist->GetNetRecords();
        ASSERT_GT(netListNetRecords.size(), 0);
        //for (auto& netRecord : netListNetRecords)
        //{
        //    if (netRecord->netName == "")
        //    {

        //    }
        //}    
	}

    TEST_F(FileArchiveLoadFixture, Test_SampleDesign_Can_Has_Data)
    {
        auto rigidFlexDesignPath = getDesignPath("sample_design.tgz");

        Odb::Lib::FileModel::Design::FileArchive rigidFlexOdbDesign(rigidFlexDesignPath.string());
        auto success = rigidFlexOdbDesign.ParseFileModel();
        ASSERT_TRUE(success);

        const auto& findStep = rigidFlexOdbDesign.GetStepsByName().find("step");
        ASSERT_NE(findStep, rigidFlexOdbDesign.GetStepsByName().end());

        // step
        auto& pStep = findStep->second;
        auto name = pStep->GetName();
        ASSERT_STREQ(name.c_str(), "step");

        // eda data
        auto& edaData = pStep->GetEdaDataFile();
        auto& netRecords = edaData.GetNetRecords();
        ASSERT_GT(netRecords.size(), 20);

        auto& pNetRecord = netRecords[20];
        ASSERT_GT(pNetRecord->m_subnetRecords.size(), 44);
        auto& pSubnetRecord = pNetRecord->m_subnetRecords[44];

        auto subnetType = pSubnetRecord->type;
        ASSERT_EQ(subnetType, Odb::Lib::FileModel::Design::EdaDataFile::NetRecord::SubnetRecord::Type::Trace);
        //auto pToeprintSubnetRecord = pSubnetRecord;
        //auto viaType = pToeprintSubnetRecord->type;
        //if (viaType == Odb::Lib::FileModel::Design::EdaDataFile::NetRecord::ToeprintSubnetRecord::Type::Via)
        //{
        //}        

        auto& packageRecords = edaData.GetPackageRecords();
        ASSERT_GT(packageRecords.size(), 0);
        auto& pPackageRecord = packageRecords[0];
        const auto& packageName = pPackageRecord->name;
        // assert package name

        // layers
        auto& layersByName = pStep->GetLayersByName();
        ASSERT_GT(layersByName.size(), 0);

        auto layerFind = layersByName.find(Odb::Lib::FileModel::Design::ComponentsFile::TOP_COMPONENTS_LAYER_NAME);
        ASSERT_NE(layerFind, layersByName.end());
        auto& pLayer = layerFind->second;
        ASSERT_NE(pLayer, nullptr);
        auto layerName = pLayer->GetName();
        // assert layer name

        // netlist
        const auto& netlistsByName = pStep->GetNetlistsByName();
        ASSERT_GT(netlistsByName.size(), 0);

        auto netlistFind = netlistsByName.find("cadnet");
        ASSERT_NE(netlistFind, netlistsByName.end());
        auto& pNetlist = netlistFind->second;
        ASSERT_NE(pNetlist, nullptr);

        auto netlistName = pNetlist->GetName();
        // assert netlist name

        auto& netListNetRecords = pNetlist->GetNetRecords();
        ASSERT_GT(netListNetRecords.size(), 0);
        //for (auto& netRecord : netListNetRecords)
        //{
        //    if (netRecord->netName == "")
        //    {

        //    }
        //}    
    }
}