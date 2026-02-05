#include <gtest/gtest.h>
#include "Fixtures/FileArchiveLoadFixture.h"
#include <gmock/gmock-matchers.h>
#include "FileModel/Design/FileArchive.h"
#include <string>
#include "FileModel/Design/ComponentsFile.h"

using namespace Odb::Lib::FileModel;
using namespace Odb::Lib::FileModel::Design;
using namespace Odb::Test::Fixtures;
using namespace testing;

namespace
{
    struct ComponentLookup
    {
        const ComponentsFile *componentsFile{nullptr};
        std::shared_ptr<ComponentsFile::ComponentRecord> record;
        std::vector<std::string> attributeNames;
    };

    ComponentLookup FindComponent(const FileArchive &fileArchive, const std::string &compName)
    {
        ComponentLookup result;

        const auto &stepsByName = fileArchive.GetStepsByName();
        for (const auto &kvStep : stepsByName)
        {
            const auto &step = kvStep.second;
            const auto &layersByName = step->GetLayersByName();
            for (const auto &kvLayer : layersByName)
            {
                const auto &layer = kvLayer.second;
                const auto &componentsFile = layer->GetComponentsFile();
                for (const auto &record : componentsFile.GetComponentRecords())
                {
                    if (record->compName == compName)
                    {
                        result.componentsFile = &componentsFile;
                        result.record = record;
                        result.attributeNames = componentsFile.GetAttributeNames();
                        return result;
                    }
                }
            }
        }

        return result;
    }
}

namespace Odb::Test
{
    // Validates the full pipeline for a known component with height defined at attribute index 0.
    TEST_F(FileArchiveLoadFixture, ComponentHeight_Pipeline_sample_design_R70_preserved)
    {
        auto pFileArchive = m_pDesignCache->GetFileArchive("sample_design");
        ASSERT_THAT(pFileArchive, NotNull());

        auto lookup = FindComponent(*pFileArchive, "R70");
        ASSERT_NE(nullptr, lookup.componentsFile) << "Component R70 not found";
        ASSERT_THAT(lookup.record, NotNull());

        // Parse stage: attribute names and lookup table in-memory
        ASSERT_FALSE(lookup.attributeNames.empty());
        EXPECT_EQ(".comp_height", lookup.attributeNames.front());

        const auto &table = lookup.record->GetAttributeLookupTable();
        ASSERT_TRUE(table.find("0") != table.end());
        EXPECT_EQ("0.022000", table.at("0"));

        // Protobuf stage
        auto proto = lookup.componentsFile->to_protobuf();
        ASSERT_THAT(proto, NotNull());

        bool foundInProto = false;
        std::string heightProto;
        for (const auto &recMsg : proto->componentrecords())
        {
            if (recMsg.compname() == "R70")
            {
                foundInProto = true;
                auto it = recMsg.attributelookuptable().find("0");
                ASSERT_NE(recMsg.attributelookuptable().end(), it);
                heightProto = it->second;
                break;
            }
        }
        ASSERT_TRUE(foundInProto);
        EXPECT_EQ("0.022000", heightProto);

        // JSON stage (REST payload uses to_json on the ComponentsFile)
        auto json = lookup.componentsFile->to_json();
        ASSERT_FALSE(json.empty());
        EXPECT_NE(std::string::npos, json.find("\"attributeLookupTable\""));
        EXPECT_NE(std::string::npos, json.find("\"0\":\"" + heightProto + "\""));
    }
}
