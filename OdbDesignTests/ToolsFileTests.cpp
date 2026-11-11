// Clean-room restoration tests: ToolsFile (drill tools) + matrix layer color.
//
// Behavioral verification per docs/plan/clean-room-reimplementation-plan.md §4:
// real-design parses, Save->re-Parse protobuf round-trips, spec-legality
// negative cases, and the FINISH_SIZE=-1 sentinel. No string-presence checks.

#include "FileModel/Design/ToolsFile.h"
#include "FileModel/Design/MatrixFile.h"
#include "FileModel/Design/RgbColor.h"
#include "FileModel/Design/FileArchive.h"
#include "FileModel/parse_error.h"
#include "Fixtures/TestDataFixture.h"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <sstream>
#include <array>
#include <vector>

namespace fs = std::filesystem;
using namespace Odb::Lib::FileModel::Design;

namespace
{
	// each test writes into its own subdirectory (keyed by test name): the
	// suite registers via gtest_discover_tests, so parallel ctest runs each
	// test as its own process and a shared path would race
	fs::path writeTempToolsFile(const std::string& contents, const std::string& tag)
	{
		auto dir = fs::temp_directory_path() / "odbdesign_toolsfile_tests" / tag;
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
		"}\n", "synthetic");

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
		"}\n", "illegal_subtype");

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
		"}\n", "unknown_type");

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
		"}\n", "empty_finish");

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
		"}\n", "roundtrip");

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

TEST(ToolsFileTests, Parse_MalformedNumericField_ThrowsParseError)
{
	// hostile/malformed numeric values surface as parse_error with context —
	// never an escaping exception, never a silently-wrong partial parse
	const std::vector<std::pair<std::string, std::string>> badValues = {
		{"NUM", "not-a-number"},
		{"NUM", "5abc"},              // trailing garbage
		{"NUM", "-5"},                // sign on unsigned field
		{"NUM", "4294967296"},        // > UINT_MAX (LP64 truncation class)
		{"FINISH_SIZE", "12abc"},
		{"THICKNESS", "1e999"},
	};
	int caseIndex = 0;
	for (const auto& [key, badValue] : badValues)
	{
		std::string contents;
		if (key == "THICKNESS")
		{
			contents = "UNITS=INCH\n" + key + "=" + badValue + "\n";
		}
		else
		{
			contents = "UNITS=INCH\nTOOLS {\n    " + key + "=" + badValue + "\n    TYPE=VIA\n}\n";
		}
		const auto dir = writeTempToolsFile(contents, "malformed_" + std::to_string(caseIndex++));
		ToolsFile toolsFile;
		ASSERT_THROW(toolsFile.Parse(dir), Odb::Lib::FileModel::parse_error)
			<< key << "=" << badValue << " was not rejected";
	}
}

TEST(ToolsFileTests, Parse_ToolsPrefixedGarbageLine_ThrowsParseError)
{
	// "TOOLS" followed by anything other than "{" is not a record open; it
	// must surface as a parse error, not silently start a record
	const auto dir = writeTempToolsFile(
		"UNITS=INCH\n"
		"TOOLS_GARBAGE {\n"
		"    NUM=1\n"
		"}\n", "tools_prefix_garbage");

	ToolsFile toolsFile;
	ASSERT_THROW(toolsFile.Parse(dir), Odb::Lib::FileModel::parse_error);
}

TEST(RgbColorTests, StringRoundTrip_PreservesComponents)
{
	// explicit colors: to_string must emit the six-character zero-padded
	// decimal form from_string parses (the (6,6,6) -> "666" regression)
	const std::vector<std::array<uint8_t, 3>> explicitColors = {
		{60, 60, 90}, {0, 0, 0}, {6, 6, 6}, {9, 5, 99},
	};
	for (const auto& rgb : explicitColors)
	{
		const auto six = [](int v) {
			return std::string(1, '0' + v / 10) + std::string(1, '0' + v % 10);
		};
		RgbColor original;
		ASSERT_TRUE(original.from_string(six(rgb[0]) + six(rgb[1]) + six(rgb[2])));
		EXPECT_FALSE(original.noPreference);

		RgbColor reparsed;
		ASSERT_TRUE(reparsed.from_string(original.to_string()))
			<< "to_string output not re-parsable: " << original.to_string();
		EXPECT_EQ(reparsed.red, original.red);
		EXPECT_EQ(reparsed.green, original.green);
		EXPECT_EQ(reparsed.blue, original.blue);
		EXPECT_EQ(reparsed.noPreference, original.noPreference);
	}

	// no-preference sentinel round-trips
	RgbColor noPref("0");
	EXPECT_TRUE(noPref.noPreference);
	RgbColor reparsedNoPref;
	EXPECT_TRUE(reparsedNoPref.from_string(noPref.to_string()));
	EXPECT_TRUE(reparsedNoPref.noPreference);

	// non-numeric component is rejected (returns false), not an escaping
	// exception
	RgbColor bad;
	EXPECT_FALSE(bad.from_string("ABCDEF"));
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
