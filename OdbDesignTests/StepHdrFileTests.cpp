// Specification-driven tests for StepHdrFile (<step_name>/stephdr).
//
// ODB++ spec 8.1 U4, "Step Entities / <step_name>/stephdr (Step Header)",
// pp. 139-141: the file consists of general fields (X_DATUM, Y_DATUM, ID,
// X_ORIGIN, Y_ORIGIN, TOP_ACTIVE, BOTTOM_ACTIVE, RIGHT_ACTIVE, LEFT_ACTIVE,
// AFFECTING_BOM, AFFECTING_BOM_CHANGED, ONLINE_<type>) and an array of
// STEP-REPEAT records (NAME, X, Y, DX, DY, NX, NY, ANGLE, FLIP, MIRROR)
// that "exists only if there are nested steps". The spec's example file
// opens each record with "STEP-REPEAT {" on one line and closes with "}".
//
// Every test writes a synthetic stephdr file into its own temp directory,
// parses it through the real StepHdrFile::Parse, and asserts independently
// computed values. StepHdrFile exposes no field getters, so the parsed
// state is observed through to_protobuf() (the production serialization
// path) and re-materialized with from_protobuf() for round-trip checks.
// Protobuf maps are only ever iterated, never find()/at()'d (issue #599).
//
// No Save() coverage: StepHdrFile::Save is a no-op stub (plan §5.8).

#include "FileModel/Design/StepHdrFile.h"
#include "FileModel/parse_error.h"
#include "FileModel/invalid_odb_error.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;
using Odb::Lib::FileModel::Design::StepHdrFile;
using ProtobufStepHdrFile = Odb::Lib::Protobuf::StepHdrFile;

namespace
{
	// Each test gets its own directory keyed by tag: gtest_discover_tests
	// runs every test as its own process, so a shared path would race.
	fs::path writeStepHdr(const std::string& contents, const std::string& tag)
	{
		auto dir = fs::temp_directory_path() / "odbdesign_stephdr_tests" / tag;
		fs::create_directories(dir);
		std::ofstream out(dir / "stephdr", std::ios::trunc);
		out << contents;
		return dir;
	}

	// Snapshot a protobuf onlineValues map into a std::map by iteration only
	// (never find()/at() on a google::protobuf::Map -- issue #599).
	std::map<std::string, std::string> snapshotOnlineValues(const ProtobufStepHdrFile& message)
	{
		std::map<std::string, std::string> values;
		for (const auto& kv : message.onlinevalues())
		{
			values[kv.first] = kv.second;
		}
		return values;
	}
}

TEST(StepHdrFileTests, Parse_AllGeneralScalars_ExtractsExactValues)
{
	// Independent expectations straight from the literal file below.
	const auto dir = writeStepHdr(
		"X_DATUM=0.25\n"
		"Y_DATUM=-1.5\n"
		"ID=123456\n"
		"X_ORIGIN=2.0\n"
		"Y_ORIGIN=3.25\n"
		"TOP_ACTIVE=1\n"
		"BOTTOM_ACTIVE=0.125\n"
		"RIGHT_ACTIVE=4.5\n"
		"LEFT_ACTIVE=8.125\n"
		"AFFECTING_BOM=0\n"
		"AFFECTING_BOM_CHANGED=1\n", "general_scalars");

	StepHdrFile stepHdr;
	ASSERT_TRUE(stepHdr.Parse(dir));
	auto message = stepHdr.to_protobuf();
	ASSERT_NE(message, nullptr);

	EXPECT_DOUBLE_EQ(message->xdatum(), 0.25);
	EXPECT_DOUBLE_EQ(message->ydatum(), -1.5);
	EXPECT_EQ(message->id(), 123456);
	EXPECT_DOUBLE_EQ(message->xorigin(), 2.0);
	EXPECT_DOUBLE_EQ(message->yorigin(), 3.25);
	EXPECT_DOUBLE_EQ(message->topactive(), 1.0);
	EXPECT_DOUBLE_EQ(message->bottomactive(), 0.125);
	EXPECT_DOUBLE_EQ(message->rightactive(), 4.5);
	EXPECT_DOUBLE_EQ(message->leftactive(), 8.125);
	EXPECT_EQ(message->affectingbom(), "0");
	EXPECT_TRUE(message->affectingbomchanged());
	EXPECT_EQ(message->steprepeatrecords_size(), 0);
}

TEST(StepHdrFileTests, Parse_LowercaseKeyVariants_ParsedSameAsUppercase)
{
	// The parser accepts the lowercase spelling of every general key.
	const auto dir = writeStepHdr(
		"x_datum=1.5\n"
		"y_datum=2.5\n"
		"id=77\n"
		"x_origin=0.0\n"
		"y_origin=0.0\n"
		"top_active=1\n"
		"bottom_active=2\n"
		"right_active=3\n"
		"left_active=4\n"
		"affecting_bom=1\n"
		"affecting_bom_changed=0\n", "lowercase_keys");

	StepHdrFile stepHdr;
	ASSERT_TRUE(stepHdr.Parse(dir));
	auto message = stepHdr.to_protobuf();

	EXPECT_DOUBLE_EQ(message->xdatum(), 1.5);
	EXPECT_DOUBLE_EQ(message->ydatum(), 2.5);
	EXPECT_EQ(message->id(), 77);
	EXPECT_DOUBLE_EQ(message->topactive(), 1.0);
	EXPECT_DOUBLE_EQ(message->bottomactive(), 2.0);
	EXPECT_DOUBLE_EQ(message->rightactive(), 3.0);
	EXPECT_DOUBLE_EQ(message->leftactive(), 4.0);
	EXPECT_EQ(message->affectingbom(), "1");
	EXPECT_FALSE(message->affectingbomchanged());
}

TEST(StepHdrFileTests, Parse_AffectingBomChangedZeroOne_MapToFalseTrue)
{
	// spec: the field contains the integer "0" or "1".
	const auto dir = writeStepHdr("AFFECTING_BOM_CHANGED=0\n", "bom_changed_zero");
	StepHdrFile stepHdr;
	ASSERT_TRUE(stepHdr.Parse(dir));
	EXPECT_FALSE(stepHdr.to_protobuf()->affectingbomchanged());

	const auto dir1 = writeStepHdr("AFFECTING_BOM_CHANGED=1\n", "bom_changed_one");
	StepHdrFile stepHdr1;
	ASSERT_TRUE(stepHdr1.Parse(dir1));
	EXPECT_TRUE(stepHdr1.to_protobuf()->affectingbomchanged());
}

TEST(StepHdrFileTests, Parse_OnlinePrefixedKeys_CollectedIntoOnlineValuesMap)
{
	// spec p.140: "ONLINE_<online_type>" -- obsolete backward-compatibility
	// parameters, "any number of the DRC_* or NET_* parameters". They are
	// read through the same name=value machinery as the other general
	// fields (a space-separated form is silently ignored -- see
	// Parse_ValuelessAssignment).
	const auto dir = writeStepHdr(
		"X_DATUM=0\n"
		"ONLINE_DRC_PADTO=abreast\n"
		"ONLINE_NET_LENGTH=12.5\n"
		"ONLINE_MIN_MASK_SLIVER=0.05\n", "online_values");

	StepHdrFile stepHdr;
	ASSERT_TRUE(stepHdr.Parse(dir));
	auto message = stepHdr.to_protobuf();

	EXPECT_EQ(message->onlinevalues_size(), 3);
	const auto values = snapshotOnlineValues(*message);
	ASSERT_EQ(values.size(), 3U);
	EXPECT_EQ(values.at("ONLINE_DRC_PADTO"), "abreast");
	EXPECT_EQ(values.at("ONLINE_NET_LENGTH"), "12.5");
	EXPECT_EQ(values.at("ONLINE_MIN_MASK_SLIVER"), "0.05");

	// scalar parse and ONLINE collection must not interfere
	EXPECT_DOUBLE_EQ(message->xdatum(), 0.0);
}

TEST(StepHdrFileTests, Parse_InlineStepRepeatRecord_AllFieldsExtracted)
{
	// Field values and their meaning come from the spec's STEP-REPEAT table
	// (p.140-141), not from the parser.
	const auto dir = writeStepHdr(
		"# example shape from ODB++ spec p.141\n"
		"UNITS=INCH\n"
		"STEP-REPEAT {\n"
		"NAME=1UP\n"
		"X=1.5\n"
		"Y=1.6\n"
		"DX=1.2\n"
		"DY=1.2\n"
		"NX=6\n"
		"NY=6\n"
		"ANGLE=0\n"
		"FLIP=NO\n"
		"MIRROR=NO\n"
		"}\n", "inline_record");

	StepHdrFile stepHdr;
	ASSERT_TRUE(stepHdr.Parse(dir));
	auto message = stepHdr.to_protobuf();

	ASSERT_EQ(message->steprepeatrecords_size(), 1);
	const auto& record = message->steprepeatrecords(0);
	EXPECT_EQ(record.name(), "1UP");
	EXPECT_DOUBLE_EQ(record.x(), 1.5);
	// note: the parser reads these with std::stof (float) into double
	// members, so non-float-exact decimals carry ~1e-7 relative error --
	// documented precision loss, asserted with float-level tolerance here
	EXPECT_NEAR(record.y(), 1.6, 1e-6);
	EXPECT_NEAR(record.dx(), 1.2, 1e-6);
	EXPECT_NEAR(record.dy(), 1.2, 1e-6);
	EXPECT_EQ(record.nx(), 6);
	EXPECT_EQ(record.ny(), 6);
	EXPECT_DOUBLE_EQ(record.angle(), 0.0);
	EXPECT_FALSE(record.flip());
	EXPECT_FALSE(record.mirror());
}

TEST(StepHdrFileTests, Parse_NegativeAndFractionalGeometry_PreservedExactly)
{
	// panel-placed steps commonly sit at negative coordinates
	const auto dir = writeStepHdr(
		"STEP-REPEAT {\n"
		"NAME=corner_au\n"
		"X=-10.75\n"
		"Y=-0.001\n"
		"DX=2.5\n"
		"DY=3.0\n"
		"NX=1\n"
		"NY=1\n"
		"ANGLE=45.5\n"
		"FLIP=YES\n"
		"MIRROR=YES\n"
		"}\n", "negative_geometry");

	StepHdrFile stepHdr;
	ASSERT_TRUE(stepHdr.Parse(dir));
	auto message = stepHdr.to_protobuf();

	ASSERT_EQ(message->steprepeatrecords_size(), 1);
	const auto& record = message->steprepeatrecords(0);
	EXPECT_DOUBLE_EQ(record.x(), -10.75);
	EXPECT_NEAR(record.y(), -0.001, 1e-6); // stof(float) precision, see note above
	EXPECT_DOUBLE_EQ(record.dx(), 2.5);
	EXPECT_DOUBLE_EQ(record.dy(), 3.0);
	EXPECT_EQ(record.nx(), 1);
	EXPECT_EQ(record.ny(), 1);
	EXPECT_DOUBLE_EQ(record.angle(), 45.5);
	EXPECT_TRUE(record.flip());
	EXPECT_TRUE(record.mirror());
}

TEST(StepHdrFileTests, Parse_LowercaseRecordKeysAndFlags_ParsedSame)
{
	const auto dir = writeStepHdr(
		"STEP-REPEAT {\n"
		"name=pcb\n"
		"x=1\n"
		"y=2\n"
		"dx=3\n"
		"dy=4\n"
		"nx=2\n"
		"ny=3\n"
		"angle=90\n"
		"flip=yes\n"
		"mirror=no\n"
		"}\n", "lowercase_record");

	StepHdrFile stepHdr;
	ASSERT_TRUE(stepHdr.Parse(dir));
	auto message = stepHdr.to_protobuf();

	ASSERT_EQ(message->steprepeatrecords_size(), 1);
	const auto& record = message->steprepeatrecords(0);
	EXPECT_EQ(record.name(), "pcb");
	EXPECT_DOUBLE_EQ(record.x(), 1.0);
	EXPECT_DOUBLE_EQ(record.y(), 2.0);
	EXPECT_DOUBLE_EQ(record.dx(), 3.0);
	EXPECT_DOUBLE_EQ(record.dy(), 4.0);
	EXPECT_EQ(record.nx(), 2);
	EXPECT_EQ(record.ny(), 3);
	EXPECT_DOUBLE_EQ(record.angle(), 90.0);
	EXPECT_TRUE(record.flip());
	EXPECT_FALSE(record.mirror());
}

TEST(StepHdrFileTests, Parse_MultipleStepRepeatRecords_AllCollectedInOrder)
{
	// two panelized rows: first flip+mirror, second clean
	const auto dir = writeStepHdr(
		"STEP-REPEAT {\n"
		"NAME=left_col\n"
		"X=0\n"
		"Y=0\n"
		"DX=5\n"
		"DY=5\n"
		"NX=1\n"
		"NY=2\n"
		"ANGLE=0\n"
		"FLIP=YES\n"
		"MIRROR=NO\n"
		"}\n"
		"\n"
		"STEP-REPEAT {\n"
		"NAME=right_col\n"
		"X=12\n"
		"Y=0\n"
		"DX=5\n"
		"DY=5\n"
		"NX=1\n"
		"NY=2\n"
		"ANGLE=270\n"
		"FLIP=NO\n"
		"MIRROR=NO\n"
		"}\n", "multiple_records");

	StepHdrFile stepHdr;
	ASSERT_TRUE(stepHdr.Parse(dir));
	auto message = stepHdr.to_protobuf();

	ASSERT_EQ(message->steprepeatrecords_size(), 2);
	EXPECT_EQ(message->steprepeatrecords(0).name(), "left_col");
	EXPECT_EQ(message->steprepeatrecords(0).ny(), 2);
	EXPECT_TRUE(message->steprepeatrecords(0).flip());
	EXPECT_EQ(message->steprepeatrecords(1).name(), "right_col");
	EXPECT_DOUBLE_EQ(message->steprepeatrecords(1).x(), 12.0);
	EXPECT_DOUBLE_EQ(message->steprepeatrecords(1).angle(), 270.0);
	EXPECT_FALSE(message->steprepeatrecords(1).flip());
}

TEST(StepHdrFileTests, Parse_EmptyRecord_GetsSpecDefaults)
{
	// a record with no assignments keeps every field at its zero value
	// (all stephdr attributes are optional per the spec's field tables)
	const auto dir = writeStepHdr(
		"STEP-REPEAT {\n"
		"}\n", "empty_record");

	StepHdrFile stepHdr;
	ASSERT_TRUE(stepHdr.Parse(dir));
	auto message = stepHdr.to_protobuf();

	ASSERT_EQ(message->steprepeatrecords_size(), 1);
	const auto& record = message->steprepeatrecords(0);
	EXPECT_EQ(record.name(), "");
	EXPECT_DOUBLE_EQ(record.x(), 0.0);
	EXPECT_DOUBLE_EQ(record.y(), 0.0);
	EXPECT_EQ(record.nx(), 0);
	EXPECT_EQ(record.ny(), 0);
	EXPECT_FALSE(record.flip());
	EXPECT_FALSE(record.mirror());
}

TEST(StepHdrFileTests, Parse_GeneralFieldsAroundRecord_ScopedCorrectly)
{
	// the spec's example file (p.141) has general fields before and after
	// the STEP-REPEAT array; the record must not swallow them and vice versa
	const auto dir = writeStepHdr(
		"X_DATUM=1.0\n"
		"Y_DATUM=2.0\n"
		"ID=4242\n"
		"STEP-REPEAT {\n"
		"NAME=1UP\n"
		"X=0.0\n"
		"Y=0.0\n"
		"DX=1.0\n"
		"DY=1.0\n"
		"NX=1\n"
		"NY=1\n"
		"ANGLE=0\n"
		"FLIP=NO\n"
		"MIRROR=NO\n"
		"}\n"
		"TOP_ACTIVE=1\n"
		"BOTTOM_ACTIVE=1\n"
		"RIGHT_ACTIVE=1\n"
		"LEFT_ACTIVE=1\n"
		"AFFECTING_BOM=\n"
		"AFFECTING_BOM_CHANGED=0\n", "fields_around_record");

	StepHdrFile stepHdr;
	ASSERT_TRUE(stepHdr.Parse(dir));
	auto message = stepHdr.to_protobuf();

	EXPECT_DOUBLE_EQ(message->xdatum(), 1.0);
	EXPECT_DOUBLE_EQ(message->ydatum(), 2.0);
	EXPECT_EQ(message->id(), 4242);
	EXPECT_DOUBLE_EQ(message->topactive(), 1.0);
	EXPECT_DOUBLE_EQ(message->bottomactive(), 1.0);
	EXPECT_DOUBLE_EQ(message->rightactive(), 1.0);
	EXPECT_DOUBLE_EQ(message->leftactive(), 1.0);
	EXPECT_EQ(message->affectingbom(), "");
	EXPECT_FALSE(message->affectingbomchanged());
	ASSERT_EQ(message->steprepeatrecords_size(), 1);
	EXPECT_EQ(message->steprepeatrecords(0).name(), "1UP");
}

TEST(StepHdrFileTests, ProtobufRoundTrip_PreservesScalarsRecordsAndOnlineMap)
{
	const auto dir = writeStepHdr(
		"X_DATUM=3.5\n"
		"Y_DATUM=-2.25\n"
		"ID=998877\n"
		"X_ORIGIN=0.5\n"
		"Y_ORIGIN=0.75\n"
		"TOP_ACTIVE=1\n"
		"BOTTOM_ACTIVE=2\n"
		"RIGHT_ACTIVE=3\n"
		"LEFT_ACTIVE=4\n"
		"AFFECTING_BOM=main_bom\n"
		"AFFECTING_BOM_CHANGED=1\n"
		"ONLINE_DRC_X=7\n"
		"STEP-REPEAT {\n"
		"NAME=piece_a\n"
		"X=1.5\n"
		"Y=2.5\n"
		"DX=10\n"
		"DY=20\n"
		"NX=3\n"
		"NY=4\n"
		"ANGLE=33.25\n"
		"FLIP=YES\n"
		"MIRROR=NO\n"
		"}\n"
		"STEP-REPEAT {\n"
		"NAME=piece_b\n"
		"X=-1\n"
		"Y=-2\n"
		"DX=0.5\n"
		"DY=0.25\n"
		"NX=1\n"
		"NY=1\n"
		"ANGLE=-15\n"
		"FLIP=NO\n"
		"MIRROR=YES\n"
		"}\n", "protobuf_roundtrip");

	StepHdrFile original;
	ASSERT_TRUE(original.Parse(dir));
	auto originalMessage = original.to_protobuf();

	StepHdrFile roundTripped;
	roundTripped.from_protobuf(*originalMessage);
	auto roundTrippedMessage = roundTripped.to_protobuf();

	EXPECT_DOUBLE_EQ(roundTrippedMessage->xdatum(), 3.5);
	EXPECT_DOUBLE_EQ(roundTrippedMessage->ydatum(), -2.25);
	EXPECT_EQ(roundTrippedMessage->id(), 998877);
	EXPECT_DOUBLE_EQ(roundTrippedMessage->xorigin(), 0.5);
	EXPECT_DOUBLE_EQ(roundTrippedMessage->yorigin(), 0.75);
	EXPECT_DOUBLE_EQ(roundTrippedMessage->topactive(), 1.0);
	EXPECT_DOUBLE_EQ(roundTrippedMessage->bottomactive(), 2.0);
	EXPECT_DOUBLE_EQ(roundTrippedMessage->rightactive(), 3.0);
	EXPECT_DOUBLE_EQ(roundTrippedMessage->leftactive(), 4.0);
	EXPECT_EQ(roundTrippedMessage->affectingbom(), "main_bom");
	EXPECT_TRUE(roundTrippedMessage->affectingbomchanged());

	ASSERT_EQ(roundTrippedMessage->steprepeatrecords_size(), 2);
	const auto& a = roundTrippedMessage->steprepeatrecords(0);
	EXPECT_EQ(a.name(), "piece_a");
	EXPECT_DOUBLE_EQ(a.x(), 1.5);
	EXPECT_DOUBLE_EQ(a.y(), 2.5);
	EXPECT_DOUBLE_EQ(a.dx(), 10.0);
	EXPECT_DOUBLE_EQ(a.dy(), 20.0);
	EXPECT_EQ(a.nx(), 3);
	EXPECT_EQ(a.ny(), 4);
	EXPECT_DOUBLE_EQ(a.angle(), 33.25);
	EXPECT_TRUE(a.flip());
	EXPECT_FALSE(a.mirror());
	const auto& b = roundTrippedMessage->steprepeatrecords(1);
	EXPECT_EQ(b.name(), "piece_b");
	EXPECT_DOUBLE_EQ(b.x(), -1.0);
	EXPECT_DOUBLE_EQ(b.y(), -2.0);
	EXPECT_DOUBLE_EQ(b.angle(), -15.0);
	EXPECT_FALSE(b.flip());
	EXPECT_TRUE(b.mirror());

	// map equality via iteration (never protobuf-Map find())
	const auto originalValues = snapshotOnlineValues(*originalMessage);
	const auto roundTrippedValues = snapshotOnlineValues(*roundTrippedMessage);
	ASSERT_EQ(roundTrippedValues.size(), originalValues.size());
	for (const auto& kv : originalValues)
	{
		ASSERT_NE(roundTrippedValues.find(kv.first), roundTrippedValues.end())
			<< "online key lost in round trip: " << kv.first;
		EXPECT_EQ(roundTrippedValues.at(kv.first), kv.second);
	}
}

TEST(StepHdrFileTests, ProtobufRoundTrip_AbsentOptionals_StayAtZeroValueDefaults)
{
	// a file with a single field: every other optional stays absent and the
	// round trip must not invent values for it
	const auto dir = writeStepHdr("X_DATUM=5.0\n", "optional_absence");

	StepHdrFile original;
	ASSERT_TRUE(original.Parse(dir));
	auto message = original.to_protobuf();

	EXPECT_DOUBLE_EQ(message->xdatum(), 5.0);
	EXPECT_DOUBLE_EQ(message->ydatum(), 0.0);
	EXPECT_EQ(message->id(), 0);
	EXPECT_FALSE(message->affectingbomchanged());
	EXPECT_EQ(message->affectingbom(), "");
	EXPECT_EQ(message->onlinevalues_size(), 0);
	EXPECT_EQ(message->steprepeatrecords_size(), 0);

	StepHdrFile roundTripped;
	roundTripped.from_protobuf(*message);
	auto roundTrippedMessage = roundTripped.to_protobuf();

	EXPECT_DOUBLE_EQ(roundTrippedMessage->xdatum(), 5.0);
	EXPECT_EQ(roundTrippedMessage->id(), 0);
	EXPECT_EQ(roundTrippedMessage->steprepeatrecords_size(), 0);
}

TEST(StepHdrFileTests, Parse_MissingDirectory_ReturnsFalse)
{
	// OdbFile::Parse requires the directory to exist; no exception
	StepHdrFile stepHdr;
	EXPECT_FALSE(stepHdr.Parse(fs::temp_directory_path() / "odbdesign_stephdr_missing_dir_xyz"));
}

TEST(StepHdrFileTests, Parse_MissingStepHdrFile_ThrowsInvalidOdbError)
{
	// an existing (empty) step directory without a stephdr file
	const auto dir = fs::temp_directory_path() / "odbdesign_stephdr_tests" / "no_stephdr";
	fs::create_directories(dir);

	StepHdrFile stepHdr;
	EXPECT_THROW(stepHdr.Parse(dir), Odb::Lib::FileModel::invalid_odb_error);
}

TEST(StepHdrFileTests, Parse_UnknownGeneralKey_ThrowsParseError)
{
	const auto dir = writeStepHdr("TOTALLY_UNKNOWN_KEY=1\n", "unknown_general_key");

	StepHdrFile stepHdr;
	EXPECT_THROW(stepHdr.Parse(dir), Odb::Lib::FileModel::parse_error);
}

TEST(StepHdrFileTests, Parse_GeneralKeyInsideRecord_ThrowsParseError)
{
	// X_DATUM is a general field; inside the STEP-REPEAT array only the ten
	// record fields are legal (spec p.140-141 tables)
	const auto dir = writeStepHdr(
		"STEP-REPEAT {\n"
		"NAME=1UP\n"
		"X_DATUM=5\n"
		"}\n", "general_key_in_record");

	StepHdrFile stepHdr;
	EXPECT_THROW(stepHdr.Parse(dir), Odb::Lib::FileModel::parse_error);
}

TEST(StepHdrFileTests, Parse_UnknownKeyInsideRecord_ThrowsParseError)
{
	const auto dir = writeStepHdr(
		"STEP-REPEAT {\n"
		"NAME=1UP\n"
		"WEIGHT=999\n"
		"}\n", "unknown_key_in_record");

	StepHdrFile stepHdr;
	EXPECT_THROW(stepHdr.Parse(dir), Odb::Lib::FileModel::parse_error);
}

TEST(StepHdrFileTests, Parse_RecordKeyOutsideRecord_ThrowsParseError)
{
	// NAME is only legal inside the STEP-REPEAT array; at file scope the
	// general-field branch does not recognize it
	const auto dir = writeStepHdr("NAME=loose\n", "record_key_outside");

	StepHdrFile stepHdr;
	EXPECT_THROW(stepHdr.Parse(dir), Odb::Lib::FileModel::parse_error);
}

TEST(StepHdrFileTests, Parse_InvalidFlipValue_ThrowsParseError)
{
	// spec: FLIP/MIRROR take YES or NO; anything else must be rejected
	for (const auto& [tag, line] : std::vector<std::pair<std::string, std::string>>{
			 { "flip_upper", "FLIP=MAYBE\n" },
			 { "flip_lower", "flip=TRUE\n" },
			 { "mirror_upper", "MIRROR=MAYBE\n" },
			 { "mirror_lower", "mirror=1\n" } })
	{
		const auto dir = writeStepHdr(
			"STEP-REPEAT {\n" + line + "}\n", "invalid_flag_" + tag);
		StepHdrFile stepHdr;
		EXPECT_THROW(stepHdr.Parse(dir), Odb::Lib::FileModel::parse_error)
			<< line << " was not rejected";
	}
}

TEST(StepHdrFileTests, Parse_NonNumericGeneralField_ThrowsInvalidArgument)
{
	// std::stof/stoi throw std::invalid_argument for non-numeric payloads;
	// StepHdrFile's catch clauses (parse_error, invalid_odb_error) do not
	// intercept it, so the exception reaches the caller raw -- documented
	// contract: a hostile value never parses successfully
	const auto dir = writeStepHdr("X_DATUM=abc\n", "bad_number_general");

	StepHdrFile stepHdr;
	EXPECT_THROW(stepHdr.Parse(dir), std::invalid_argument);
}

TEST(StepHdrFileTests, Parse_NonNumericRecordField_ThrowsInvalidArgument)
{
	const auto dir = writeStepHdr(
		"STEP-REPEAT {\n"
		"X=not-a-number\n"
		"}\n", "bad_number_record");

	StepHdrFile stepHdr;
	EXPECT_THROW(stepHdr.Parse(dir), std::invalid_argument);
}

TEST(StepHdrFileTests, Parse_IntegerOverflowOnNx_ThrowsOutOfRange)
{
	// NX is read with std::stoi: a value beyond int range raises
	// std::out_of_range (note: "1e999" would NOT throw -- stoi's prefix
	// semantics stop at 'e' -- so the overflow class is covered here)
	const auto dir = writeStepHdr(
		"STEP-REPEAT {\n"
		"NX=99999999999999\n"
		"}\n", "nx_overflow");

	StepHdrFile stepHdr;
	EXPECT_THROW(stepHdr.Parse(dir), std::out_of_range);
}

TEST(StepHdrFileTests, Parse_NumericPrefixTrailingGarbage_SilentlyAccepted)
{
	// DOCUMENTED DIVERGENCE from strict numeric parsing: std::stof/stoi
	// prefix semantics accept "5abc" as 5 (ToolsFile's from_chars-based
	// parser rejects the same input). Pinned here so a switch to strict
	// parsing fails this test loudly instead of silently changing behavior.
	const auto dir = writeStepHdr(
		"STEP-REPEAT {\n"
		"X=1.5abc\n"
		"NX=7xyz\n"
		"}\n", "numeric_prefix");

	StepHdrFile stepHdr;
	ASSERT_TRUE(stepHdr.Parse(dir));
	auto message = stepHdr.to_protobuf();
	ASSERT_EQ(message->steprepeatrecords_size(), 1);
	EXPECT_DOUBLE_EQ(message->steprepeatrecords(0).x(), 1.5);
	EXPECT_EQ(message->steprepeatrecords(0).nx(), 7);
}

TEST(StepHdrFileTests, Parse_CloseBraceWithoutOpenRecord_ThrowsParseError)
{
	const auto dir = writeStepHdr("}\n", "orphan_close");

	StepHdrFile stepHdr;
	EXPECT_THROW(stepHdr.Parse(dir), Odb::Lib::FileModel::parse_error);
}

TEST(StepHdrFileTests, Parse_OpenBraceWithoutStepRepeatHeader_ThrowsParseError)
{
	// a bare '{' at file scope: no record is open, so this cannot be a
	// step or layer array element
	const auto dir = writeStepHdr(
		"{\n"
		"NAME=1UP\n"
		"}\n", "orphan_open");

	StepHdrFile stepHdr;
	EXPECT_THROW(stepHdr.Parse(dir), Odb::Lib::FileModel::parse_error);
}

TEST(StepHdrFileTests, Parse_StepRepeatHeaderBraceOnFollowingLine_ThrowsParseError)
{
	// The spec's example (p.141) opens records with "STEP-REPEAT {" on one
	// line. A brace on the following line hits the generic array-open branch
	// with no current record, which is a parse error under the current
	// parser -- pinned so a future "brace on next line" support change is
	// a deliberate decision, not an accident.
	const auto dir = writeStepHdr(
		"STEP-REPEAT\n"
		"{\n"
		"NAME=1UP\n"
		"}\n", "brace_next_line");

	StepHdrFile stepHdr;
	EXPECT_THROW(stepHdr.Parse(dir), Odb::Lib::FileModel::parse_error);
}

TEST(StepHdrFileTests, Parse_UnclosedRecordAtEof_RecordSilentlyDropped)
{
	// DOCUMENTED CONTRACT (defect): the parser has no EOF finalization --
	// a STEP-REPEAT record whose '}' never arrives is discarded silently
	// and Parse still returns true. Pinned so the data-loss behavior is
	// visible and any fix must consciously update this expectation.
	const auto dir = writeStepHdr(
		"STEP-REPEAT {\n"
		"NAME=never_closed\n"
		"X=1\n", "unclosed_record");

	StepHdrFile stepHdr;
	ASSERT_TRUE(stepHdr.Parse(dir));
	EXPECT_EQ(stepHdr.to_protobuf()->steprepeatrecords_size(), 0);
}

TEST(StepHdrFileTests, Parse_ValuelessAssignment_LineIgnored)
{
	// "X=" with nothing after '=' fails the value getline, so the line is
	// neither an error nor an assignment (contrast: "X=abc" throws)
	const auto dir = writeStepHdr(
		"X_DATUM=1.0\n"
		"Y_DATUM=\n", "valueless_assignment");

	StepHdrFile stepHdr;
	ASSERT_TRUE(stepHdr.Parse(dir));
	auto message = stepHdr.to_protobuf();
	EXPECT_DOUBLE_EQ(message->xdatum(), 1.0);
	EXPECT_DOUBLE_EQ(message->ydatum(), 0.0); // assignment never happened
}

TEST(StepHdrFileTests, Parse_CommentsAndBlankLines_Skipped)
{
	const auto dir = writeStepHdr(
		"# leading comment\n"
		"\n"
		"X_DATUM=2.0\n"
		"# a comment with = sign and STEP-REPEAT text inside\n"
		"\n"
		"Y_DATUM=3.0\n"
		"\n", "comments");

	StepHdrFile stepHdr;
	ASSERT_TRUE(stepHdr.Parse(dir));
	auto message = stepHdr.to_protobuf();
	EXPECT_DOUBLE_EQ(message->xdatum(), 2.0);
	EXPECT_DOUBLE_EQ(message->ydatum(), 3.0);
	EXPECT_EQ(message->steprepeatrecords_size(), 0);
}
