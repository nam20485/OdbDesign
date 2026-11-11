// Clean-room restoration tests: ToolsFile (drill tools) + matrix layer color.
//
// Behavioral verification per docs/plan/clean-room-reimplementation-plan.md §4:
// real-design parses, Save->re-Parse protobuf round-trips, spec-legality
// negative cases, and the FINISH_SIZE=-1 sentinel. No string-presence checks.

#include "FileModel/Design/ToolsFile.h"
#include "FileModel/Design/MatrixFile.h"
#include "FileModel/parse_error.h"
#include "Fixtures/TestDataFixture.h"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <sstream>

namespace fs = std::filesystem;
using namespace Odb::Lib::FileModel::Design;

namespace
{
	fs::path writeTempToolsFile(const std::string& contents)
	{
		auto dir = fs::temp_directory_path() / "odbdesign_toolsfile_tests";
		fs::create_directories(dir);
		auto path = dir / "tools";
		std::ofstream out(path, std::ios::trunc);
		out << contents;
		return dir;
	}

	// protobuf equality via serialized bytes (SerializeAsString may append
	// nondeterministic map ordering; these messages hold no maps)
	bool sameProtobuf(const Odb::Lib::Protobuf::ToolsFile& lhs, const Odb::Lib::Protobuf::ToolsFile& rhs)
	{
		return lhs.SerializeAsString() == rhs.SerializeAsString();
	}
}

TEST(ToolsFileTests, Parse_SyntheticGrammar_ExtractsFields)
{
	const auto dir = writeTempToolsFile(
		"UNITS=INCH\n"
		"THICKNESS=62.5\n"
		"USER_PARAMS=method25\n"
		"# comment line\n"
		"TOOLS {\n"
		"    NUM=1\n"
		"    TYPE=VIA\n"
		"    TYPE2=LASER\n"
		"    MIN_TOL=0.5\n"
		"    MAX_TOL=1.5\n"
		"    BIT=D3\n"
		"    FINISH_SIZE=11.5\n"
		"    DRILL_SIZE=13.5\n"
		"}\n"
		"TOOLS {\n"
		"    NUM=2\n"
		"    TYPE=PLATED\n"
		"    MIN_TOL=0\n"
		"    MAX_TOL=0\n"
		"    BIT=\n"
		"    FINISH_SIZE=15\n"
		"    DRILL_SIZE=19\n"
		"}\n");

	ToolsFile toolsFile;
	ASSERT_TRUE(toolsFile.Parse(dir));

	EXPECT_EQ(toolsFile.GetUnits(), "INCH");
	EXPECT_DOUBLE_EQ(toolsFile.GetThickness(), 62.5);
	ASSERT_EQ(toolsFile.GetUserParams().size(), 1U);
	EXPECT_EQ(toolsFile.GetUserParams()[0], "method25");

	const auto& tools = toolsFile.GetTools();
	ASSERT_EQ(tools.size(), 2U);

	EXPECT_EQ(tools[0]->num, 1U);
	EXPECT_EQ(tools[0]->type, ToolsFile::ToolRecord::Type::Via);
	EXPECT_EQ(tools[0]->subType, ToolsFile::ToolRecord::SubType::Laser);
	EXPECT_DOUBLE_EQ(tools[0]->minTol, 0.5);
	EXPECT_DOUBLE_EQ(tools[0]->maxTol, 1.5);
	EXPECT_EQ(tools[0]->bit, "D3");
	EXPECT_DOUBLE_EQ(tools[0]->finishSize, 11.5);
	EXPECT_DOUBLE_EQ(tools[0]->drillSize, 13.5);

	// TYPE2 absent -> spec default STANDARD
	EXPECT_EQ(tools[1]->type, ToolsFile::ToolRecord::Type::Plated);
	EXPECT_EQ(tools[1]->subType, ToolsFile::ToolRecord::SubType::Standard);
}

TEST(ToolsFileTests, Parse_MissingFile_IsNotAnError)
{
	// non-drill layers carry no tools table
	ToolsFile toolsFile;
	const auto emptyDir = fs::temp_directory_path() / "odbdesign_toolsfile_tests_absent";
	fs::create_directories(emptyDir);
	EXPECT_TRUE(toolsFile.Parse(emptyDir));
	EXPECT_TRUE(toolsFile.GetTools().empty());
}

TEST(ToolsFileTests, Parse_IllegalSubTypeForType_ThrowsParseError)
{
	// spec: TYPE=NON_PLATED permits only TYPE2=STANDARD
	const auto dir = writeTempToolsFile(
		"UNITS=INCH\n"
		"TOOLS {\n"
		"    NUM=1\n"
		"    TYPE=NON_PLATED\n"
		"    TYPE2=PHOTO\n"
		"}\n");

	ToolsFile toolsFile;
	ASSERT_THROW(toolsFile.Parse(dir), Odb::Lib::FileModel::parse_error);
}

TEST(ToolsFileTests, Parse_UnknownType_ThrowsParseError)
{
	const auto dir = writeTempToolsFile(
		"UNITS=INCH\n"
		"TOOLS {\n"
		"    NUM=1\n"
		"    TYPE=GALVANIC\n"
		"}\n");

	ToolsFile toolsFile;
	ASSERT_THROW(toolsFile.Parse(dir), Odb::Lib::FileModel::parse_error);
}

TEST(ToolsFileTests, Parse_EmptyFinishSize_UsesSpecSentinel)
{
	const auto dir = writeTempToolsFile(
		"UNITS=INCH\n"
		"TOOLS {\n"
		"    NUM=1\n"
		"    TYPE=VIA\n"
		"    FINISH_SIZE=\n"
		"}\n");

	ToolsFile toolsFile;
	ASSERT_TRUE(toolsFile.Parse(dir));
	ASSERT_EQ(toolsFile.GetTools().size(), 1U);
	// spec: -1 when not set
	EXPECT_DOUBLE_EQ(toolsFile.GetTools()[0]->finishSize, -1.0);
}

TEST(ToolsFileTests, SaveThenReParse_ProducesIdenticalProtobuf)
{
	const auto dir = writeTempToolsFile(
		"UNITS=INCH\n"
		"THICKNESS=54.7402\n"
		"USER_PARAMS=\n"
		"TOOLS {\n"
		"    NUM=1\n"
		"    TYPE=VIA\n"
		"    MIN_TOL=0\n"
		"    MAX_TOL=0\n"
		"    BIT=\n"
		"    FINISH_SIZE=7.87402\n"
		"    DRILL_SIZE=7.87402\n"
		"}\n");

	ToolsFile first;
	ASSERT_TRUE(first.Parse(dir));
	auto pFirstMessage = first.to_protobuf();

	// Save to a fresh directory and re-parse the output
	std::ostringstream savedStream;
	ASSERT_TRUE(first.Save(savedStream));
	const auto secondDir = fs::temp_directory_path() / "odbdesign_toolsfile_tests_roundtrip";
	fs::create_directories(secondDir);
	std::ofstream outFile(secondDir / "tools", std::ios::trunc);
	outFile << savedStream.str();
	outFile.close();

	ToolsFile second;
	ASSERT_TRUE(second.Parse(secondDir));

	// path/directory metadata differ by design; compare the parsed data
	auto pSecondMessage = second.to_protobuf();
	pSecondMessage->clear_directory();
	pSecondMessage->clear_path();
	auto firstMessageData = *pFirstMessage;
	firstMessageData.clear_directory();
	firstMessageData.clear_path();

	EXPECT_TRUE(sameProtobuf(firstMessageData, *pSecondMessage));
	EXPECT_EQ(second.GetUnits(), "INCH");
	ASSERT_EQ(second.GetTools().size(), 1U);
	EXPECT_DOUBLE_EQ(second.GetTools()[0]->finishSize, 7.87402);
}

TEST(ToolsFileTests, RealDesigns_LayersCarryParsedTools)
{
	const char* testDataDir = std::getenv("ODB_TEST_DATA_DIR");
	if (testDataDir == nullptr || testDataDir[0] == '\0')
	{
		GTEST_SKIP() << "ODB_TEST_DATA_DIR not set";
	}

	// designodb_rigidflex carries 8 tools files; sample_design carries 2
	for (const auto* designName : { "designodb_rigidflex.tgz", "sample_design.tgz" })
	{
		const auto designPath = fs::path(testDataDir) / designName;
		if (!fs::exists(designPath))
		{
			GTEST_SKIP() << designName << " not in test data dir";
		}

		FileArchive archive(designPath.string());
		ASSERT_TRUE(archive.ParseFileModel()) << "failed to parse " << designName;

		// walk steps -> layers; every layer with a tools file must expose
		// parsed records through the LayerDirectory wiring
		auto toolRecordsSeen = 0U;
		for (const auto& kvStep : archive.GetStepsByName())
		{
			for (const auto& kvLayer : kvStep.second->GetLayersByName())
			{
				const auto& tools = kvLayer.second->GetToolsFile().GetTools();
				toolRecordsSeen += static_cast<unsigned int>(tools.size());
				for (const auto& pTool : tools)
				{
					// spec legality: parsed records must be well-formed
					EXPECT_TRUE(pTool->subTypeIsLegal()) << designName;
					EXPECT_NE(pTool->type, ToolsFile::ToolRecord::Type::Unknown) << designName;
				}
			}
		}

		EXPECT_GT(toolRecordsSeen, 0U) << "no tool records parsed from " << designName;
	}
}

TEST(MatrixColorTests, RealDesigns_LayerColorsRoundTripThroughProtobuf)
{
	const char* testDataDir = std::getenv("ODB_TEST_DATA_DIR");
	if (testDataDir == nullptr || testDataDir[0] == '\0')
	{
		GTEST_SKIP() << "ODB_TEST_DATA_DIR not set";
	}

	const auto designPath = fs::path(testDataDir) / "sample_design.tgz";
	if (!fs::exists(designPath))
	{
		GTEST_SKIP() << "sample_design.tgz not in test data dir";
	}

	FileArchive archive(designPath.string());
	ASSERT_TRUE(archive.ParseFileModel());

	const auto& matrix = archive.GetMatrixFile();
	const auto& layerRecords = matrix.GetLayerRecords();
	ASSERT_GT(layerRecords.size(), 0U);

	// COLOR=0 rows parse into the no-preference default; round-tripping every
	// record through protobuf must preserve the parsed component values
	for (const auto& pLayerRecord : layerRecords)
	{
		auto pMessage = pLayerRecord->to_protobuf();
		MatrixFile::LayerRecord roundTripped;
		roundTripped.from_protobuf(*pMessage);

		EXPECT_EQ(roundTripped.color.red, pLayerRecord->color.red);
		EXPECT_EQ(roundTripped.color.green, pLayerRecord->color.green);
		EXPECT_EQ(roundTripped.color.blue, pLayerRecord->color.blue);
		EXPECT_EQ(roundTripped.color.noPreference, pLayerRecord->color.noPreference);
	}
}
