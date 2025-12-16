#include <gtest/gtest.h>

#include "Fixtures/FileArchiveLoadFixture.h"

#include <enums.h>
#include <FileModel/Design/FeaturesFile.h>
#include <FileModel/Design/LayerDirectory.h>
#include <FileModel/Design/StepDirectory.h>
#include <FileModel/Design/FileArchive.h>

#include <featuresfile.pb.h>
#include <FileModel/parse_error.h>

#include <filesystem>
#include <fstream>

using namespace Odb::Lib::FileModel::Design;
using namespace Odb::Test::Fixtures;

namespace
{
	std::filesystem::path make_temp_dir(const std::string& name)
	{
		auto base = std::filesystem::temp_directory_path() / "OdbDesignTests";
		auto dir = base / name;
		std::filesystem::create_directories(dir);
		return dir;
	}

	void write_text_file(const std::filesystem::path& path, const std::string& contents)
	{
		std::ofstream out(path, std::ios::out | std::ios::trunc);
		ASSERT_TRUE(out.is_open()) << "Failed to open: " << path.string();
		out << contents;
	}

}

TEST(SymbolUnitsParsingTests, MissingSymbolUnitInheritsFromUnitsLine_WhenUnitsBeforeSymbols)
{
	auto dir = make_temp_dir("SymbolUnitsParsingTests_Before");
	write_text_file(dir / "features",
		"UNITS=INCH\n"
		"$1 sym_a\n"
		"$2 sym_b I\n");

	FeaturesFile f;
	ASSERT_TRUE(f.Parse(dir, "features"));
	ASSERT_EQ(f.GetUnits(), "INCH");

	const auto& syms = f.GetSymbolNames();
	ASSERT_EQ(syms.size(), 2u);
	ASSERT_NE(syms[0], nullptr);
	ASSERT_NE(syms[1], nullptr);

	EXPECT_EQ(syms[0]->GetUnitType(), Odb::Lib::UnitType::Imperial); // inherited
	EXPECT_EQ(syms[1]->GetUnitType(), Odb::Lib::UnitType::Imperial); // explicit
}

TEST(SymbolUnitsParsingTests, MissingSymbolUnitInheritsFromUnitsLine_WhenUnitsAfterSymbols)
{
	auto dir = make_temp_dir("SymbolUnitsParsingTests_After");
	write_text_file(dir / "features",
		"$1 sym_a\n"
		"$2 sym_b I\n"
		"UNITS=INCH\n");

	FeaturesFile f;
	ASSERT_TRUE(f.Parse(dir, "features"));
	ASSERT_EQ(f.GetUnits(), "INCH");

	const auto& syms = f.GetSymbolNames();
	ASSERT_EQ(syms.size(), 2u);
	ASSERT_NE(syms[0], nullptr);
	ASSERT_NE(syms[1], nullptr);

	EXPECT_EQ(syms[0]->GetUnitType(), Odb::Lib::UnitType::Imperial); // retroactively inherited
	EXPECT_EQ(syms[1]->GetUnitType(), Odb::Lib::UnitType::Imperial); // explicit
}

TEST(SymbolUnitsParsingTests, MissingSymbolUnitRemainsNone_WhenNoUnitsInFile)
{
	auto dir = make_temp_dir("SymbolUnitsParsingTests_None");
	write_text_file(dir / "features",
		"$1 sym_a\n"
		"$2 sym_b\n");

	FeaturesFile f;
	ASSERT_TRUE(f.Parse(dir, "features"));
	ASSERT_TRUE(f.GetUnits().empty());

	const auto& syms = f.GetSymbolNames();
	ASSERT_EQ(syms.size(), 2u);
	ASSERT_NE(syms[0], nullptr);
	ASSERT_NE(syms[1], nullptr);

	EXPECT_EQ(syms[0]->GetUnitType(), Odb::Lib::UnitType::None);
	EXPECT_EQ(syms[1]->GetUnitType(), Odb::Lib::UnitType::None);
}

TEST(SymbolUnitsParsingTests, ExplicitMetricSymbolInMetricFile)
{
	auto dir = make_temp_dir("SymbolUnitsParsingTests_Metric");
	write_text_file(dir / "features",
		"UNITS=MM\n"
		"$1 sym_metric M\n");

	FeaturesFile f;
	ASSERT_TRUE(f.Parse(dir, "features"));
	ASSERT_EQ(f.GetUnits(), "MM");

	const auto& syms = f.GetSymbolNames();
	ASSERT_EQ(syms.size(), 1u);
	ASSERT_NE(syms[0], nullptr);
	EXPECT_EQ(syms[0]->GetUnitType(), Odb::Lib::UnitType::Metric);
}

TEST(SymbolNameParsingTests, ParsesIndexNameAndUnit)
{
	SymbolName s;
	ASSERT_TRUE(s.Parse(std::filesystem::path("/tmp"), "$42 my_sym I", 1));
	EXPECT_EQ(s.GetIndex(), 42);
	EXPECT_EQ(s.GetName(), "my_sym");
	EXPECT_EQ(s.GetUnitType(), Odb::Lib::UnitType::Imperial);
}

TEST(SymbolNameParsingTests, ApplyDefaultUnitTypeIfNone)
{
	SymbolName s;
	ASSERT_TRUE(s.Parse(std::filesystem::path("/tmp"), "$1 sym_none", 1));
	EXPECT_EQ(s.GetUnitType(), Odb::Lib::UnitType::None);

	s.ApplyDefaultUnitTypeIfNone(Odb::Lib::UnitType::Metric);
	EXPECT_EQ(s.GetUnitType(), Odb::Lib::UnitType::Metric);

	// Should not overwrite once set
	s.ApplyDefaultUnitTypeIfNone(Odb::Lib::UnitType::Imperial);
	EXPECT_EQ(s.GetUnitType(), Odb::Lib::UnitType::Metric);
}

TEST(SymbolNameParsingTests, InvalidIndexTokenThrowsParseError)
{
	SymbolName s;
	EXPECT_THROW(s.Parse(std::filesystem::path("/tmp"), "$abc sym I", 1), Odb::Lib::FileModel::parse_error);
}

TEST(SymbolNameParsingTests, MissingIndexDefaultsToMinusOne)
{
	SymbolName s;
	ASSERT_TRUE(s.Parse(std::filesystem::path("/tmp"), "sym_no_index I", 1));
	EXPECT_EQ(s.GetIndex(), -1);
	EXPECT_EQ(s.GetName(), "sym_no_index");
	EXPECT_EQ(s.GetUnitType(), Odb::Lib::UnitType::Imperial);
}

TEST(FeatureRecordPadParsingTests, SymNumDefaultsFromAptDefSymbolNum)
{
	auto dir = make_temp_dir("PadParsing_SymNumFromAptDef");
	write_text_file(dir / "features",
		"UNITS=INCH\n"
		"$1 sym_a I\n"
		"P 1 2 7 P 10 0 0\n");

	FeaturesFile f;
	ASSERT_TRUE(f.Parse(dir, "features"));

	const auto& recs = f.GetFeatureRecords();
	ASSERT_EQ(recs.size(), 1u);
	ASSERT_NE(recs[0], nullptr);
	EXPECT_EQ(recs[0]->type, FeaturesFile::FeatureRecord::Type::Pad);
	EXPECT_EQ(recs[0]->apt_def_symbol_num, 7);
	EXPECT_EQ(recs[0]->sym_num, 7);
}

TEST(FeatureRecordPadParsingTests, SymNumStaysUnsetWhenAptDefSymbolNumMissing)
{
	auto dir = make_temp_dir("PadParsing_SymNumUnset");
	write_text_file(dir / "features",
		"UNITS=INCH\n"
		"$1 sym_a I\n"
		"P 0 0 -1 0.5 P 10 0 0\n");

	FeaturesFile f;
	ASSERT_TRUE(f.Parse(dir, "features"));

	const auto& recs = f.GetFeatureRecords();
	ASSERT_EQ(recs.size(), 1u);
	ASSERT_NE(recs[0], nullptr);
	EXPECT_EQ(recs[0]->apt_def_symbol_num, -1);
	EXPECT_EQ(recs[0]->sym_num, -1);
}

TEST(FeatureRecordSerializationTests, PadOptionalFieldsRespectPresence)
{
	FeaturesFile::FeatureRecord fr;
	fr.type = FeaturesFile::FeatureRecord::Type::Pad;
	fr.sym_num = -1;
	fr.apt_def_symbol_num = -1;

	auto pb = fr.to_protobuf();
	ASSERT_NE(pb, nullptr);
	EXPECT_FALSE(pb->has_sym_num());
	EXPECT_FALSE(pb->has_apt_def_symbol_num());

	fr.sym_num = 5;
	fr.apt_def_symbol_num = 5;
	auto pb2 = fr.to_protobuf();
	ASSERT_NE(pb2, nullptr);
	EXPECT_TRUE(pb2->has_sym_num());
	EXPECT_TRUE(pb2->has_apt_def_symbol_num());
}

TEST(FeatureRecordSerializationTests, SymNumIsPresenceSignificant)
{
	auto fr = std::make_shared<FeaturesFile::FeatureRecord>();
	fr->type = FeaturesFile::FeatureRecord::Type::Line;
	fr->sym_num = -1;
	fr->apt_def_symbol_num = -1;

	auto pb = fr->to_protobuf();
	ASSERT_NE(pb, nullptr);
	EXPECT_FALSE(pb->has_sym_num());
	EXPECT_FALSE(pb->has_apt_def_symbol_num());

	fr->sym_num = 0;
	fr->apt_def_symbol_num = 0;
	auto pb2 = fr->to_protobuf();
	ASSERT_NE(pb2, nullptr);
	EXPECT_TRUE(pb2->has_sym_num());
	EXPECT_TRUE(pb2->has_apt_def_symbol_num());
}

TEST(FeatureRecordSerializationTests, FromProtobufRestoresMissingAsMinusOne)
{
	Odb::Lib::Protobuf::FeaturesFile::FeatureRecord msg;
	msg.set_type(Odb::Lib::Protobuf::FeaturesFile::FeatureRecord::Line);
	// sym_num and apt_def_symbol_num intentionally unset

	FeaturesFile::FeatureRecord fr;
	fr.from_protobuf(msg);
	EXPECT_EQ(fr.sym_num, -1);
	EXPECT_EQ(fr.apt_def_symbol_num, -1);

	msg.set_sym_num(9);
	msg.set_apt_def_symbol_num(11);
	fr.from_protobuf(msg);
	EXPECT_EQ(fr.sym_num, 9);
	EXPECT_EQ(fr.apt_def_symbol_num, 11);
}

TEST_F(FileArchiveLoadFixture, RigidFlex_Signal2_AllSymbolsResolveToImperial)
{
	auto pFileArchive = m_pDesignCache->GetFileArchive("designodb_rigidflex");
	ASSERT_NE(pFileArchive, nullptr);

	auto pStep = pFileArchive->GetStepDirectory("cellular_flip-phone");
	ASSERT_NE(pStep, nullptr);

	const auto& layers = pStep->GetLayersByName();
	auto it = layers.find("signal_2");
	ASSERT_NE(it, layers.end());

	const auto& features = it->second->GetFeaturesFile();
	ASSERT_EQ(features.GetUnits(), "INCH");

	int symbolCount = 0;
	for (const auto& sym : features.GetSymbolNames())
	{
		if (!sym) continue;
		symbolCount++;
		EXPECT_EQ(sym->GetUnitType(), Odb::Lib::UnitType::Imperial) << "Symbol name=" << sym->GetName();
	}
	EXPECT_GT(symbolCount, 0);
}

TEST_F(FileArchiveLoadFixture, PadsUseSymNum_MatchesAptDefSymbolNum_WhenPresent)
{
	auto assert_design_step = [&](const std::string& designName, const std::string& stepName)
	{
		auto pFileArchive = m_pDesignCache->GetFileArchive(designName);
		ASSERT_NE(pFileArchive, nullptr) << designName;

		auto pStep = pFileArchive->GetStepDirectory(stepName);
		ASSERT_NE(pStep, nullptr) << stepName;

		int checkedPads = 0;
		for (const auto& kv : pStep->GetLayersByName())
		{
			const auto& layerName = kv.first;
			const auto& ff = kv.second->GetFeaturesFile();
			for (const auto& fr : ff.GetFeatureRecords())
			{
				if (!fr) continue;
				if (fr->type != FeaturesFile::FeatureRecord::Type::Pad) continue;
				if (fr->apt_def_symbol_num < 0) continue;
				checkedPads++;
				EXPECT_EQ(fr->sym_num, fr->apt_def_symbol_num) << "design=" << designName << " step=" << stepName << " layer=" << layerName;
			}
		}

		EXPECT_GT(checkedPads, 0) << "No pads with apt_def_symbol_num found for " << designName << "/" << stepName;
	};

	assert_design_step("sample_design", "step");
	assert_design_step("designodb_rigidflex", "cellular_flip-phone");
}
