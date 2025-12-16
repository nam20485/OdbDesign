#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <set>
#include "Fixtures/FileArchiveLoadFixture.h"
#include "FileModel/Design/MatrixFile.h"

using namespace Odb::Test::Fixtures;
using namespace testing;
using Odb::Lib::FileModel::Design::MatrixFile;

namespace Odb::Test
{
    class MatrixFileTests : public FileArchiveLoadFixture
    {
    };

    TEST_F(MatrixFileTests, OptionalAttributeRecognition)
    {
        EXPECT_TRUE(MatrixFile::attributeValueIsOptional("OLD_NAME"));
        EXPECT_TRUE(MatrixFile::attributeValueIsOptional("old_name"));
        EXPECT_TRUE(MatrixFile::attributeValueIsOptional("START_NAME"));
        EXPECT_TRUE(MatrixFile::attributeValueIsOptional("END_NAME"));
        EXPECT_TRUE(MatrixFile::attributeValueIsOptional("ID"));
        EXPECT_FALSE(MatrixFile::attributeValueIsOptional("ROW"));
        EXPECT_FALSE(MatrixFile::attributeValueIsOptional("NAME"));
        EXPECT_FALSE(MatrixFile::attributeValueIsOptional("TYPE"));
    }

    TEST_F(MatrixFileTests, StepRecordsMatchStepDirectoryKeys)
    {
        auto pFileArchive = m_pDesignCache->GetFileArchive("sample_design");
        ASSERT_NE(pFileArchive, nullptr);

        const auto& stepDirectory = pFileArchive->GetStepsByName();
        const auto& stepRecords = pFileArchive->GetMatrixFile().GetStepRecords();

        ASSERT_FALSE(stepDirectory.empty());
        ASSERT_FALSE(stepRecords.empty());

        std::set<std::string> matrixStepNames;
        for (const auto& rec : stepRecords)
        {
            ASSERT_NE(rec, nullptr);
            matrixStepNames.insert(rec->name);
        }

        for (const auto& kv : stepDirectory)
        {
            EXPECT_THAT(matrixStepNames, Contains(kv.first)) << "Matrix file missing step " << kv.first;
        }
    }

    TEST_F(MatrixFileTests, LayerRecordsHaveUniqueNonEmptyNames)
    {
        auto pFileArchive = m_pDesignCache->GetFileArchive("designodb_rigidflex");
        ASSERT_NE(pFileArchive, nullptr);

        const auto& layers = pFileArchive->GetMatrixFile().GetLayerRecords();
        ASSERT_FALSE(layers.empty());

        std::set<std::string> names;
        for (const auto& layer : layers)
        {
            ASSERT_NE(layer, nullptr);
            EXPECT_FALSE(layer->name.empty());
            EXPECT_TRUE(names.insert(layer->name).second) << "Duplicate layer name: " << layer->name;
        }
    }

    TEST_F(MatrixFileTests, ProtoRoundTripPreservesCounts)
    {
        auto pFileArchive = m_pDesignCache->GetFileArchive("designodb_rigidflex");
        ASSERT_NE(pFileArchive, nullptr);

        const auto& matrix = pFileArchive->GetMatrixFile();
        auto pb = matrix.to_protobuf();
        ASSERT_NE(pb, nullptr);

        MatrixFile roundTripped;
        roundTripped.from_protobuf(*pb);

        EXPECT_EQ(roundTripped.GetStepRecords().size(), matrix.GetStepRecords().size());
        EXPECT_EQ(roundTripped.GetLayerRecords().size(), matrix.GetLayerRecords().size());
    }
}
