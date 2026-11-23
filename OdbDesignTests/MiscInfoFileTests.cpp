// Specification-driven tests for MiscInfoFile (misc/info).
//
// ODB++ spec 8.1 U4, "Product Model Entities / misc/info (Miscellaneous
// Information)", pp. 79-80: attribute=value lines carrying the product
// model name, job name, ODB version major/minor, ODB source, creation and
// save dates ("yyyymmdd.hhmmss"), save app/user, units and MAX_UID.
//
// Every test writes a synthetic misc/info file into its own temp
// directory, parses it through the real MiscInfoFile::Parse, and asserts
// independently computed values (timestamp expectations are derived via
// the standard library's mktime with the same local-time convention as
// Utils::parse_timestamp -- not by echoing the parser). Protobuf
// round-trips cover the message surface; per plan §5.8 the no-op Save
// contract of OTHER parsers is not invoked here, and MiscInfoFile's own
// Save is exercised once below specifically to pin that it does NOT
// round-trip (a documented defect).

#include "FileModel/Design/MiscInfoFile.h"
#include "FileModel/invalid_odb_error.h"

#include <gtest/gtest.h>

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;
using Odb::Lib::FileModel::Design::MiscInfoFile;

namespace
{
	fs::path writeInfo(const std::string& contents, const std::string& tag)
	{
		auto dir = fs::temp_directory_path() / "odbdesign_miscinfo_tests" / tag;
		fs::create_directories(dir);
		std::ofstream out(dir / "info", std::ios::trunc);
		out << contents;
		return dir;
	}

	// independent expectation for "yyyymmdd.hhmmss" under local-time
	// semantics. tm_isdst is pinned to 0 to match parse_timestamp's
	// convention (it zero-initializes its struct tm and never sets
	// isdst), which interprets wall times inside a DST window as standard
	// time -- i.e. DST-period dates resolve one hour later than a
	// DST-aware conversion would (noted as an observation in the PR).
	std::chrono::system_clock::time_point expectedTimestamp(int year, int month, int day,
		int hour, int minute, int second)
	{
		std::tm tm{};
		tm.tm_year = year - 1900;
		tm.tm_mon = month - 1;
		tm.tm_mday = day;
		tm.tm_hour = hour;
		tm.tm_min = minute;
		tm.tm_sec = second;
		tm.tm_isdst = 0;
		return std::chrono::system_clock::from_time_t(std::mktime(&tm));
	}
}

TEST(MiscInfoFileTests, Parse_AllScalarAttributes_Extracted)
{
	// attribute spellings follow the spec's misc/info example (p.79-80)
	const auto dir = writeInfo(
		"PRODUCT_MODEL_NAME=rigidflex_a\n"
		"JOB_NAME=JOB1234567\n"
		"ODB_VERSION_MAJOR=8\n"
		"ODB_VERSION_MINOR=1\n"
		"ODB_SOURCE=Mentor Boardstation database\n"
		"SAVE_APP=ODB++\n"
		"SAVE_USER=nmill\n"
		"UNITS=INCH\n"
		"MAX_UID=2470\n", "all_scalars");

	MiscInfoFile info;
	ASSERT_TRUE(info.Parse(dir));

	EXPECT_EQ(info.GetProductModelName(), "rigidflex_a");
	EXPECT_EQ(info.GetJobName(), "JOB1234567");
	EXPECT_EQ(info.GetOdbVersionMajor(), "8");
	EXPECT_EQ(info.GetOdbVersionMinor(), "1");
	EXPECT_EQ(info.GetOdbSource(), "Mentor Boardstation database");
	EXPECT_EQ(info.GetSaveApp(), "ODB++");
	EXPECT_EQ(info.GetSaveUser(), "nmill");
	EXPECT_EQ(info.GetUnits(), "INCH");
	EXPECT_EQ(info.GetMaxUniqueId(), 2470U);
}

TEST(MiscInfoFileTests, Parse_LowercaseVariants_ParsedSameAsUppercase)
{
	const auto dir = writeInfo(
		"product_model_name=pm\n"
		"job_name=job\n"
		"odb_version_major=7\n"
		"odb_version_minor=0\n"
		"odb_source=X\n"
		"save_app=Y\n"
		"save_user=Z\n"
		"units=MM\n"
		"max_uid=9\n", "lowercase");

	MiscInfoFile info;
	ASSERT_TRUE(info.Parse(dir));

	EXPECT_EQ(info.GetProductModelName(), "pm");
	EXPECT_EQ(info.GetJobName(), "job");
	EXPECT_EQ(info.GetOdbVersionMajor(), "7");
	EXPECT_EQ(info.GetOdbVersionMinor(), "0");
	EXPECT_EQ(info.GetOdbSource(), "X");
	EXPECT_EQ(info.GetSaveApp(), "Y");
	EXPECT_EQ(info.GetSaveUser(), "Z");
	EXPECT_EQ(info.GetUnits(), "MM");
	EXPECT_EQ(info.GetMaxUniqueId(), 9U);
}

TEST(MiscInfoFileTests, Parse_Timestamps_MatchIndependentMktimeExpectation)
{
	// spec p.80: dates are "yyyymmdd.hhmmss"
	const auto dir = writeInfo(
		"CREATION_DATE=20161024.101454\n"
		"SAVE_DATE=20170810.132829\n", "timestamps");

	MiscInfoFile info;
	ASSERT_TRUE(info.Parse(dir));

	const auto expectedCreation = expectedTimestamp(2016, 10, 24, 10, 14, 54);
	const auto expectedSave = expectedTimestamp(2017, 8, 10, 13, 28, 29);

	EXPECT_EQ(info.GetCreationDate(), expectedCreation);
	EXPECT_EQ(info.GetSaveDate(), expectedSave);

	// protobuf exposes the same instants as google.protobuf.Timestamp
	auto message = info.to_protobuf();
	EXPECT_EQ(message->creationdatedate().seconds(),
		std::chrono::duration_cast<std::chrono::seconds>(expectedCreation.time_since_epoch()).count());
	EXPECT_EQ(message->savedate().seconds(),
		std::chrono::duration_cast<std::chrono::seconds>(expectedSave.time_since_epoch()).count());
}

TEST(MiscInfoFileTests, Parse_UnknownAttribute_WarnedAndSkipped)
{
	// unknown keys are logged, not fatal (parser comment: "DO NOT fail
	// parsing on unknown attributes")
	const auto dir = writeInfo(
		"MYSTERY_FIELD=42\n"
		"JOB_NAME=still-parses\n", "unknown_attr");

	MiscInfoFile info;
	ASSERT_TRUE(info.Parse(dir));
	EXPECT_EQ(info.GetJobName(), "still-parses");
}

TEST(MiscInfoFileTests, Parse_MaxUidAbsent_DefaultSentinel)
{
	// MAX_UID is the optional attribute (see OPTIONAL_ATTRIBUTES); absent
	// leaves the constructor sentinel (unsigned)-1
	const auto dir = writeInfo("JOB_NAME=j\n", "no_max_uid");

	MiscInfoFile info;
	ASSERT_TRUE(info.Parse(dir));
	EXPECT_EQ(info.GetMaxUniqueId(), ~0U);
}

TEST(MiscInfoFileTests, Parse_CommentsAndBlankLines_Skipped)
{
	const auto dir = writeInfo(
		"# a comment\n"
		"\n"
		"JOB_NAME=after-comment\n"
		"# another with = signs\n"
		"\n", "comments");

	MiscInfoFile info;
	ASSERT_TRUE(info.Parse(dir));
	EXPECT_EQ(info.GetJobName(), "after-comment");
}

TEST(MiscInfoFileTests, ProtobufRoundTrip_PreservesAllFields)
{
	const auto dir = writeInfo(
		"PRODUCT_MODEL_NAME=pm_rt\n"
		"JOB_NAME=job_rt\n"
		"ODB_VERSION_MAJOR=8\n"
		"ODB_VERSION_MINOR=1\n"
		"ODB_SOURCE=src\n"
		"CREATION_DATE=20161024.101454\n"
		"SAVE_DATE=20170810.132829\n"
		"SAVE_APP=app\n"
		"SAVE_USER=user\n"
		"UNITS=MM\n"
		"MAX_UID=4321\n", "protobuf_roundtrip");

	MiscInfoFile original;
	ASSERT_TRUE(original.Parse(dir));
	auto message = original.to_protobuf();
	ASSERT_NE(message, nullptr);

	MiscInfoFile roundTripped;
	roundTripped.from_protobuf(*message);

	EXPECT_EQ(roundTripped.GetProductModelName(), "pm_rt");
	EXPECT_EQ(roundTripped.GetJobName(), "job_rt");
	EXPECT_EQ(roundTripped.GetOdbVersionMajor(), "8");
	EXPECT_EQ(roundTripped.GetOdbVersionMinor(), "1");
	EXPECT_EQ(roundTripped.GetOdbSource(), "src");
	EXPECT_EQ(roundTripped.GetCreationDate(), original.GetCreationDate());
	EXPECT_EQ(roundTripped.GetSaveDate(), original.GetSaveDate());
	EXPECT_EQ(roundTripped.GetSaveApp(), "app");
	EXPECT_EQ(roundTripped.GetSaveUser(), "user");
	EXPECT_EQ(roundTripped.GetUnits(), "MM");
	EXPECT_EQ(roundTripped.GetMaxUniqueId(), 4321U);

	// serializing both sides yields identical protobuf content
	auto roundTrippedMessage = roundTripped.to_protobuf();
	EXPECT_EQ(message->SerializeAsString(), roundTrippedMessage->SerializeAsString());
}

TEST(MiscInfoFileTests, SaveThenReParse_DefaultMaxUidSentinel_MakesFileUnparseable)
{
	// DOCUMENTED DEFECT: Save writes "MAX_UID=<value>" verbatim; a parsed
	// file without MAX_UID leaves the (unsigned)-1 sentinel, so the saved
	// line is "MAX_UID=4294967295", which Parse reads with std::stoi --
	// an int -- and throws std::out_of_range (sliced to std::exception by
	// the generic handler). Save output must be re-readable by Parse.
	MiscInfoFile original; // never parsed: MAX_UID sentinel in effect
	std::ostringstream saved;
	ASSERT_TRUE(original.Save(saved));

	const auto dir2 = fs::temp_directory_path() / "odbdesign_miscinfo_tests" / "save_sentinel_out";
	fs::create_directories(dir2);
	{
		std::ofstream out(dir2 / "info", std::ios::trunc);
		out << saved.str();
	}

	MiscInfoFile reparsed;
	EXPECT_THROW(reparsed.Parse(dir2), std::exception);
}

TEST(MiscInfoFileTests, SaveThenReParse_TimestampsDoNotRoundTrip)
{
	// DOCUMENTED DEFECT: Save emits make_timestamp ("%D %T" + millis,
	// e.g. "10/24/16 10:14:54.000") while Parse expects "%Y%m%d.%H%MSS";
	// the written file is NOT re-parseable by the same class. The re-parse
	// silently falls back to a garbage time instead of failing. (MAX_UID
	// is pinned here so the stoi overflow in the sentinel test does not
	// mask the timestamp behavior.) Pinned so a Save-format fix flips
	// this deliberately.
	const auto dir = writeInfo(
		"JOB_NAME=rt\n"
		"MAX_UID=4321\n"
		"CREATION_DATE=20161024.101454\n", "save_reparse");

	MiscInfoFile original;
	ASSERT_TRUE(original.Parse(dir));

	std::ostringstream saved;
	ASSERT_TRUE(original.Save(saved));

	const auto dir2 = fs::temp_directory_path() / "odbdesign_miscinfo_tests" / "save_reparse_out";
	fs::create_directories(dir2);
	{
		std::ofstream out(dir2 / "info", std::ios::trunc);
		out << saved.str();
	}

	MiscInfoFile reparsed;
	ASSERT_TRUE(reparsed.Parse(dir2));
	EXPECT_EQ(reparsed.GetJobName(), "rt");
	// the timestamp did NOT survive the writer/reader pair
	EXPECT_NE(reparsed.GetCreationDate(), original.GetCreationDate());
}

TEST(MiscInfoFileTests, Parse_MissingInfoFile_ThrowsInvalidOdbError)
{
	const auto dir = fs::temp_directory_path() / "odbdesign_miscinfo_tests" / "empty_dir";
	fs::create_directories(dir);

	// MiscInfoFile re-throws the concrete invalid_odb_error (its catch
	// clause matches the concrete type, unlike FeaturesFile/EdaDataFile)
	MiscInfoFile info;
	EXPECT_THROW(info.Parse(dir), Odb::Lib::FileModel::invalid_odb_error);
}
