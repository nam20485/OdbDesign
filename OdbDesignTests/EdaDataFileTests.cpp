// Specification-driven tests for EdaDataFile (<step_name>/eda/data).
//
// ODB++ spec 8.1 U4, "Step Entities / eda/data (EDA Data)", pp. 106-115:
// HDR file header record, UNITS directive (required), LYR layer names,
// PRP property records, @/& net attribute lookup tables, NET electrical
// net records, SNT subnet records (TOP/VIA/TRC/PLN, p.111), FID feature
// id records (p.112), PKG package records (p.112), PIN pin records
// (p.113), FGR feature group records (p.114) and the RC/CR/SQ/CT/OB/OS/
// OC/OE/CE outline records (p.114).
//
// Every test writes a synthetic eda/data file into its own temp directory,
// parses it through the real EdaDataFile::Parse, and asserts values
// computed independently from the spec grammar. Per plan §5.8, Save() is
// not exercised (it is a no-op stub). Protobuf maps are only ever
// iterated, never find()/at()'d (issue #599).
//
// NOTE: an orphan "SNT" line (no preceding NET) dereferences a null
// current-net pointer inside the parser (EdaDataFile.cpp:687) -- that
// crash class is reported to the maintainer and deliberately NOT
// exercised here.

#include "FileModel/Design/EdaDataFile.h"
#include "FileModel/Design/ContourPolygon.h"
#include "FileModel/parse_error.h"
#include "FileModel/invalid_odb_error.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace Odb::Lib::FileModel::Design;

namespace
{
	fs::path writeEdaData(const std::string& contents, const std::string& tag)
	{
		auto dir = fs::temp_directory_path() / "odbdesign_edadata_tests" / tag;
		fs::create_directories(dir);
		std::ofstream out(dir / "data", std::ios::trunc);
		out << contents;
		return dir;
	}

	std::map<std::string, std::string> snapshotAttributes(const AttributeLookupTable& owner)
	{
		std::map<std::string, std::string> attributes;
		for (const auto& kv : owner.GetAttributeLookupTable())
		{
			attributes[kv.first] = kv.second;
		}
		return attributes;
	}
}

TEST(EdaDataFileTests, Parse_HeaderUnitsLayerNames_Extracted)
{
	// spec example (p.106): "HDR Mentor Boardstation database", "UNITS=MM",
	// "LYR sst sst+1 sigt ..." (layer names are whitespace separated)
	const auto dir = writeEdaData(
		"#\n"
		"HDR Mentor Boardstation database\n"
		"UNITS=MM\n"
		"LYR sst sst+1 sigt drill\n"
		"#\n", "header_units_layers");

	EdaDataFile eda;
	ASSERT_TRUE(eda.Parse(dir));

	EXPECT_EQ(eda.GetSource(), "Mentor Boardstation database");
	EXPECT_EQ(eda.GetUnits(), "MM");

	const auto& layerNames = eda.GetLayerNames();
	ASSERT_EQ(layerNames.size(), 4U);
	EXPECT_EQ(layerNames.at(0), "sst");
	EXPECT_EQ(layerNames.at(1), "sst+1");
	EXPECT_EQ(layerNames.at(2), "sigt");
	EXPECT_EQ(layerNames.at(3), "drill");
}

TEST(EdaDataFileTests, Parse_TopLevelPropertyRecords_NameValueAndFloats)
{
	// PRP grammar: name, an optional quoted string value, optional floats.
	// The parser stores the FIRST data token as the string value and any
	// remaining numeric tokens as floatValues (a multi-word quoted value
	// is not reassembled -- see PR notes).
	const auto dir = writeEdaData(
		"UNITS=MM\n"
		"PRP DRILL_ORIGIN ''\n"
		"PRP FIXED_COMPONENT_LOCATION 'M3,m1' 1\n"
		"PRP ECLASS_RISE_TIME 1.5\n", "top_level_prp");

	EdaDataFile eda;
	ASSERT_TRUE(eda.Parse(dir));

	const auto& properties = eda.GetPropertyRecords();
	ASSERT_EQ(properties.size(), 3U);

	EXPECT_EQ(properties.at(0)->name, "DRILL_ORIGIN");
	EXPECT_EQ(properties.at(0)->value, ""); // empty quoted string
	EXPECT_TRUE(properties.at(0)->floatValues.empty());

	EXPECT_EQ(properties.at(1)->name, "FIXED_COMPONENT_LOCATION");
	EXPECT_EQ(properties.at(1)->value, "M3,m1");
	ASSERT_EQ(properties.at(1)->floatValues.size(), 1U);
	EXPECT_DOUBLE_EQ(properties.at(1)->floatValues.at(0), 1.0);

	EXPECT_EQ(properties.at(2)->name, "ECLASS_RISE_TIME");
	EXPECT_EQ(properties.at(2)->value, "1.5"); // unquoted numeric still lands in value
	EXPECT_TRUE(properties.at(2)->floatValues.empty());
}

TEST(EdaDataFileTests, Parse_NetRecord_NameAndRealWorldIdTrailer)
{
	// Real designs (all NET rows in the local corpora) write the
	// empty-attributes trailer "NET <name> ;;ID=<id>". The NET branch
	// consumes the name's ';' itself before handing the remainder to
	// ParseAttributeLookupTable, shifting every section one left: the
	// "ID=<uid>" text lands in the attribute map under the key "ID" and
	// NetRecord::id stays unset. DOCUMENTED DEFECT, pinned exactly.
	const auto dir = writeEdaData(
		"UNITS=MM\n"
		"NET /D_CL_TX_CLK ;;ID=1030\n", "net_record");

	EdaDataFile eda;
	ASSERT_TRUE(eda.Parse(dir));

	const auto& nets = eda.GetNetRecords();
	ASSERT_EQ(nets.size(), 1U);

	const auto& net = *nets.at(0);
	EXPECT_EQ(net.name, "/D_CL_TX_CLK");
	EXPECT_EQ(net.index, 0U);
	EXPECT_FALSE(net.id.has_value()); // defect: uid never reaches the typed field

	const auto attributes = snapshotAttributes(net);
	ASSERT_EQ(attributes.size(), 1U);
	EXPECT_EQ(attributes.at("ID"), "1030"); // defect: uid posing as an attribute
}

TEST(EdaDataFileTests, Parse_NetRecord_SpecAttributesForm_LosesAttributesAndId)
{
	// DOCUMENTED DEFECT: the spec example (p.110)
	// "NET /D_CL_TX_CLK ;0,2=1,3=2,5=1.500000;ID=1030" carries attribute
	// assignments plus a uid. After the NET branch strips the name's ';',
	// ParseAttributeLookupTable skips everything before the first ';'
	// (the attributes), then treats "ID=1030" as the attributes section:
	// real attributes are lost and the uid becomes a fake "ID" attribute.
	const auto dir = writeEdaData(
		"UNITS=MM\n"
		"NET /D_CL_TX_CLK ;0,2=1,3=2,5=1.500000;ID=1030\n", "net_spec_attrs");

	EdaDataFile eda;
	ASSERT_TRUE(eda.Parse(dir));

	ASSERT_EQ(eda.GetNetRecords().size(), 1U);
	const auto& net = *eda.GetNetRecords().at(0);
	EXPECT_EQ(net.name, "/D_CL_TX_CLK");
	EXPECT_FALSE(net.id.has_value());

	const auto attributes = snapshotAttributes(net);
	ASSERT_EQ(attributes.size(), 1U);
	EXPECT_EQ(attributes.at("ID"), "1030");
	EXPECT_EQ(attributes.count("0"), 0U); // BOOLEAN assignment lost
	EXPECT_EQ(attributes.count("2"), 0U); // TEXT assignment lost
	EXPECT_EQ(attributes.count("5"), 0U); // FLOAT assignment lost
}

TEST(EdaDataFileTests, Parse_MultipleNetRecords_FinishedOnNextAndAtEof)
{
	// a NET record ends when the next NET, PKG or FGR arrives, or at EOF
	const auto dir = writeEdaData(
		"UNITS=MM\n"
		"NET vcc\n"
		"NET gnd\n"
		"NET clk\n", "net_finalization");

	EdaDataFile eda;
	ASSERT_TRUE(eda.Parse(dir));

	const auto& nets = eda.GetNetRecords();
	ASSERT_EQ(nets.size(), 3U);
	EXPECT_EQ(nets.at(0)->name, "vcc");
	EXPECT_EQ(nets.at(0)->index, 0U);
	EXPECT_FALSE(nets.at(0)->id.has_value());
	EXPECT_EQ(nets.at(1)->name, "gnd");
	EXPECT_EQ(nets.at(1)->index, 1U);
	EXPECT_FALSE(nets.at(1)->id.has_value());
	EXPECT_EQ(nets.at(2)->name, "clk");
	EXPECT_EQ(nets.at(2)->index, 2U);
}

TEST(EdaDataFileTests, Parse_SubnetVariants_ViaTraceToeprint)
{
	// spec p.111: "SNT VIA", "SNT TRC" carry no additional parameters;
	// "SNT TOP <side> <comp_num> <toep_num>"
	const auto dir = writeEdaData(
		"UNITS=MM\n"
		"NET /D_CL_TX_CLK ;ID=1030\n"
		"SNT TOP T 16 0\n"
		"FID C 2 33\n"
		"FID C 10 33\n"
		"SNT VIA\n"
		"FID H 12 5\n"
		"SNT TRC\n"
		"FID C 1 7\n"
		"SNT TOP B 3 7\n", "subnet_variants");

	EdaDataFile eda;
	ASSERT_TRUE(eda.Parse(dir));

	const auto& nets = eda.GetNetRecords();
	ASSERT_EQ(nets.size(), 1U);
	const auto& net = *nets.at(0);

	// each SNT closes the previous one; the last is finalized at EOF
	ASSERT_EQ(net.m_subnetRecords.size(), 4U);

	const auto& toeprintTop = *net.m_subnetRecords.at(0);
	EXPECT_EQ(toeprintTop.type, EdaDataFile::NetRecord::SubnetRecord::Type::Toeprint);
	EXPECT_EQ(toeprintTop.side, Odb::Lib::BoardSide::Top);
	EXPECT_EQ(toeprintTop.componentNumber, 16U);
	EXPECT_EQ(toeprintTop.toeprintNumber, 0U);
	EXPECT_EQ(toeprintTop.index, 0U);
	ASSERT_EQ(toeprintTop.m_featureIdRecords.size(), 2U);
	EXPECT_EQ(toeprintTop.m_featureIdRecords.at(0)->type, EdaDataFile::FeatureIdRecord::Type::Copper);
	EXPECT_EQ(toeprintTop.m_featureIdRecords.at(0)->layerNumber, 2U);
	EXPECT_EQ(toeprintTop.m_featureIdRecords.at(0)->featureNumber, 33U);
	EXPECT_EQ(toeprintTop.m_featureIdRecords.at(1)->layerNumber, 10U);

	const auto& via = *net.m_subnetRecords.at(1);
	EXPECT_EQ(via.type, EdaDataFile::NetRecord::SubnetRecord::Type::Via);
	EXPECT_EQ(via.index, 1U);
	ASSERT_EQ(via.m_featureIdRecords.size(), 1U);
	EXPECT_EQ(via.m_featureIdRecords.at(0)->type, EdaDataFile::FeatureIdRecord::Type::Hole);
	EXPECT_EQ(via.m_featureIdRecords.at(0)->layerNumber, 12U);
	EXPECT_EQ(via.m_featureIdRecords.at(0)->featureNumber, 5U);

	const auto& trace = *net.m_subnetRecords.at(2);
	EXPECT_EQ(trace.type, EdaDataFile::NetRecord::SubnetRecord::Type::Trace);
	EXPECT_EQ(trace.index, 2U);
	ASSERT_EQ(trace.m_featureIdRecords.size(), 1U);
	EXPECT_EQ(trace.m_featureIdRecords.at(0)->type, EdaDataFile::FeatureIdRecord::Type::Copper);

	const auto& toeprintBottom = *net.m_subnetRecords.at(3);
	EXPECT_EQ(toeprintBottom.type, EdaDataFile::NetRecord::SubnetRecord::Type::Toeprint);
	EXPECT_EQ(toeprintBottom.side, Odb::Lib::BoardSide::Bottom);
	EXPECT_EQ(toeprintBottom.componentNumber, 3U);
	EXPECT_EQ(toeprintBottom.toeprintNumber, 7U);
	EXPECT_EQ(toeprintBottom.index, 3U);
}

TEST(EdaDataFileTests, Parse_PlaneSubnet_AllFillAndCutoutEnums)
{
	// spec p.111-112: SNT PLN <fill_type: S|O> <cutout_type: C|R|O|E> <fill_size>
	struct Case
	{
		const char* fill;
		const char* cutout;
		EdaDataFile::NetRecord::SubnetRecord::FillType expectedFill;
		EdaDataFile::NetRecord::SubnetRecord::CutoutType expectedCutout;
		double fillSize;
	};
	const std::vector<Case> cases = {
		{ "S", "C", EdaDataFile::NetRecord::SubnetRecord::FillType::Solid, EdaDataFile::NetRecord::SubnetRecord::CutoutType::Circle, 0.0 },
		{ "S", "R", EdaDataFile::NetRecord::SubnetRecord::FillType::Solid, EdaDataFile::NetRecord::SubnetRecord::CutoutType::Rectangle, 1.5 },
		{ "O", "O", EdaDataFile::NetRecord::SubnetRecord::FillType::Outline, EdaDataFile::NetRecord::SubnetRecord::CutoutType::Octagon, 2.25 },
		{ "O", "E", EdaDataFile::NetRecord::SubnetRecord::FillType::Outline, EdaDataFile::NetRecord::SubnetRecord::CutoutType::Exact, 3.0 },
	};

	int caseIndex = 0;
	for (const auto& planeCase : cases)
	{
		const auto dir = writeEdaData(
			"UNITS=MM\n"
			"NET pwr_" + std::to_string(caseIndex) + "\n"
			"SNT PLN " + planeCase.fill + " " + planeCase.cutout + " " +
				std::to_string(planeCase.fillSize) + "\n"
			"FID C 0 1\n",
			"plane_" + std::to_string(caseIndex));

		EdaDataFile eda;
		ASSERT_TRUE(eda.Parse(dir)) << "plane case " << caseIndex;
		ASSERT_EQ(eda.GetNetRecords().size(), 1U);
		const auto& net = *eda.GetNetRecords().at(0);
		ASSERT_EQ(net.m_subnetRecords.size(), 1U);

		const auto& plane = *net.m_subnetRecords.at(0);
		EXPECT_EQ(plane.type, EdaDataFile::NetRecord::SubnetRecord::Type::Plane);
		EXPECT_EQ(plane.fillType, planeCase.expectedFill);
		EXPECT_EQ(plane.cutoutType, planeCase.expectedCutout);
		EXPECT_DOUBLE_EQ(plane.fillSize, planeCase.fillSize);
		ASSERT_EQ(plane.m_featureIdRecords.size(), 1U);
		++caseIndex;
	}
}

TEST(EdaDataFileTests, Parse_MalformedSubnetLines_ThrowParseError)
{
	const std::vector<std::pair<std::string, std::string>> malformed = {
		{ "unknown_subnet_type", "SNT BOGUS\n" },
		{ "bad_fill_type", "SNT PLN X C 0\n" },
		{ "bad_cutout_type", "SNT PLN S X 0\n" },
		{ "missing_fill_size_is_tolerated_but_bad_enums_not", "SNT PLN S\n" }, // see note below
		{ "bad_side", "SNT TOP X 1 2\n" },
	};

	// SNT PLN S (no cutout/fill tokens): the cutout read fails the stream,
	// token keeps its previous value "S" which is not a legal cutout --
	// parse_error either way
	int caseIndex = 0;
	for (const auto& [tag, sntLine] : malformed)
	{
		const auto dir = writeEdaData(
			"UNITS=MM\n"
			"NET n\n"
			+ sntLine,
			"bad_snt_" + std::to_string(caseIndex++));

		EdaDataFile eda;
		EXPECT_THROW(eda.Parse(dir), Odb::Lib::FileModel::parse_error)
			<< "case " << tag << " was not rejected";
	}
}

TEST(EdaDataFileTests, Parse_OrphanFeatureId_ThrowsParseError)
{
	// FID must attach to a net subnet or a feature group (spec p.112:
	// "Each subnet record is followed by zero or more FID records")
	const auto dir = writeEdaData(
		"UNITS=MM\n"
		"FID C 1 1\n", "orphan_fid");

	EdaDataFile eda;
	EXPECT_THROW(eda.Parse(dir), Odb::Lib::FileModel::parse_error);
}

TEST(EdaDataFileTests, Parse_FidInsideNetWithoutSubnet_ThrowsParseError)
{
	const auto dir = writeEdaData(
		"UNITS=MM\n"
		"NET n\n"
		"FID C 1 1\n", "fid_without_subnet");

	EdaDataFile eda;
	EXPECT_THROW(eda.Parse(dir), Odb::Lib::FileModel::parse_error);
}

TEST(EdaDataFileTests, Parse_FeatureIdTypes_CopperLaminateHole)
{
	// spec p.112: FID type C -- Copper, L -- Laminate, H -- Hole
	struct Case
	{
		const char* token;
		EdaDataFile::FeatureIdRecord::Type expected;
	};
	const std::vector<Case> cases = {
		{ "C", EdaDataFile::FeatureIdRecord::Type::Copper },
		{ "L", EdaDataFile::FeatureIdRecord::Type::Laminate },
		{ "H", EdaDataFile::FeatureIdRecord::Type::Hole },
	};

	int caseIndex = 0;
	for (const auto& fidCase : cases)
	{
		const auto dir = writeEdaData(
			"UNITS=MM\n"
			"NET n\n"
			"SNT VIA\n"
			"FID " + std::string(fidCase.token) + " 4 9\n",
			"fid_type_" + std::to_string(caseIndex++));

		EdaDataFile eda;
		ASSERT_TRUE(eda.Parse(dir));
		ASSERT_EQ(eda.GetNetRecords().size(), 1U);
		const auto& net = *eda.GetNetRecords().at(0);
		ASSERT_EQ(net.m_subnetRecords.size(), 1U);
		ASSERT_EQ(net.m_subnetRecords.at(0)->m_featureIdRecords.size(), 1U);
		EXPECT_EQ(net.m_subnetRecords.at(0)->m_featureIdRecords.at(0)->type, fidCase.expected);
		EXPECT_EQ(net.m_subnetRecords.at(0)->m_featureIdRecords.at(0)->layerNumber, 4U);
		EXPECT_EQ(net.m_subnetRecords.at(0)->m_featureIdRecords.at(0)->featureNumber, 9U);
	}
}

TEST(EdaDataFileTests, Parse_MalformedFeatureId_ThrowsParseError)
{
	const auto dir = writeEdaData(
		"UNITS=MM\n"
		"NET n\n"
		"SNT VIA\n"
		"FID X 1 1\n", "bad_fid_type");

	EdaDataFile eda;
	EXPECT_THROW(eda.Parse(dir), Odb::Lib::FileModel::parse_error);
}

TEST(EdaDataFileTests, Parse_NetScopedProperty_AttachesToCurrentNet)
{
	// PRP binds to the innermost open record: net > package > feature group
	const auto dir = writeEdaData(
		"UNITS=MM\n"
		"NET n\n"
		"PRP NET_PROP 'yes'\n", "net_scoped_prp");

	EdaDataFile eda;
	ASSERT_TRUE(eda.Parse(dir));

	EXPECT_EQ(eda.GetPropertyRecords().size(), 0U); // not top-level
	ASSERT_EQ(eda.GetNetRecords().size(), 1U);
	const auto& net = *eda.GetNetRecords().at(0);
	ASSERT_EQ(net.m_propertyRecords.size(), 1U);
	EXPECT_EQ(net.m_propertyRecords.at(0)->name, "NET_PROP");
	EXPECT_EQ(net.m_propertyRecords.at(0)->value, "yes");
}

TEST(EdaDataFileTests, Parse_PackageRecord_BoundsAndTrailerSections)
{
	// spec p.112: PKG <name> <pitch> <xmin> <ymin> <xmax> <ymax>[;<attrs>;ID=<id>].
	// Like NET, the PKG branch consumes the ymax's ';' itself, shifting the
	// attribute sections one left: ";3=2;ID=999" arrives as "3=2;ID=999",
	// so the attribute assignment is skipped and "ID=999" is stored as a
	// fake "ID" attribute while PackageRecord::id stays unset.
	// DOCUMENTED DEFECT, pinned exactly.
	const auto dir = writeEdaData(
		"UNITS=MM\n"
		"PKG cap0402 0.05 0 0 1 1 ;3=2;ID=999\n"
		"RC -0.25 -0.1 0.5 0.2\n", "package_record");

	EdaDataFile eda;
	ASSERT_TRUE(eda.Parse(dir));

	const auto& packages = eda.GetPackageRecords();
	ASSERT_EQ(packages.size(), 1U);

	const auto& package = *packages.at(0);
	EXPECT_EQ(package.name, "cap0402");
	EXPECT_EQ(package.index, 0U);
	EXPECT_DOUBLE_EQ(package.pitch, 0.05);
	EXPECT_DOUBLE_EQ(package.xMin, 0.0);
	EXPECT_DOUBLE_EQ(package.yMin, 0.0);
	EXPECT_DOUBLE_EQ(package.xMax, 1.0);
	EXPECT_DOUBLE_EQ(package.yMax, 1.0);
	EXPECT_FALSE(package.id.has_value()); // defect: uid never reaches the typed field

	const auto attributes = snapshotAttributes(package);
	ASSERT_EQ(attributes.size(), 1U);
	EXPECT_EQ(attributes.at("ID"), "999"); // defect: uid posing as an attribute
	EXPECT_EQ(attributes.count("3"), 0U);  // attribute assignment lost

	// the RC outline before any PIN belongs to the package
	ASSERT_EQ(package.m_outlineRecords.size(), 1U);
	const auto& outline = *package.m_outlineRecords.at(0);
	EXPECT_EQ(outline.type, EdaDataFile::PackageRecord::OutlineRecord::Type::Rectangle);
	EXPECT_DOUBLE_EQ(outline.lowerLeftX, -0.25);
	EXPECT_DOUBLE_EQ(outline.lowerLeftY, -0.1);
	EXPECT_DOUBLE_EQ(outline.width, 0.5);
	EXPECT_DOUBLE_EQ(outline.height, 0.2);
}

TEST(EdaDataFileTests, Parse_PackageWithoutTrailer_IdUnset)
{
	const auto dir = writeEdaData(
		"UNITS=MM\n"
		"PKG *PDXC-L10/HX-L127W51T97 0.1 -0.25 -0.145 0.25 0.145\n", "package_no_trailer");

	EdaDataFile eda;
	ASSERT_TRUE(eda.Parse(dir));
	ASSERT_EQ(eda.GetPackageRecords().size(), 1U);
	EXPECT_FALSE(eda.GetPackageRecords().at(0)->id.has_value());
	EXPECT_EQ(eda.GetPackageRecords().at(0)->name, "*PDXC-L10/HX-L127W51T97");
}

TEST(EdaDataFileTests, Parse_PinRecords_TypesElectricalAndMountEnums)
{
	// spec p.113: PIN <name> <type: T|B|S> <xc> <yc> <fhs>
	//                   <etype: E|M|U> <mtype: S|D|T|R|P|N|H|U> [ID=<id>]
	// Each accepted mtype token in one package:
	const std::vector<std::pair<char, EdaDataFile::PackageRecord::PinRecord::MountType>> mountCases = {
		{ 'S', EdaDataFile::PackageRecord::PinRecord::MountType::Smt },
		{ 'D', EdaDataFile::PackageRecord::PinRecord::MountType::RecommendedSmtPad },
		{ 'T', EdaDataFile::PackageRecord::PinRecord::MountType::MT_ThroughHole },
		{ 'R', EdaDataFile::PackageRecord::PinRecord::MountType::RecommendedThroughHole },
		{ 'P', EdaDataFile::PackageRecord::PinRecord::MountType::Pressfit },
		{ 'N', EdaDataFile::PackageRecord::PinRecord::MountType::NonBoard },
		{ 'H', EdaDataFile::PackageRecord::PinRecord::MountType::Hole },
		{ 'U', EdaDataFile::PackageRecord::PinRecord::MountType::MT_Undefined },
	};

	std::string contents = "UNITS=MM\nPKG eightpins 0.1 0 0 1 1\n";
	for (const auto& [mount, expected] : mountCases)
	{
		contents += std::string("PIN pin_") + mount + " T 0 0 0 E " + mount + "\n";
	}
	const auto dir = writeEdaData(contents, "pin_mount_types");

	EdaDataFile eda;
	ASSERT_TRUE(eda.Parse(dir));
	ASSERT_EQ(eda.GetPackageRecords().size(), 1U);

	const auto& package = *eda.GetPackageRecords().at(0);
	ASSERT_EQ(package.m_pinRecords.size(), mountCases.size());
	for (size_t i = 0; i < mountCases.size(); ++i)
	{
		const auto& pin = *package.m_pinRecords.at(i);
		EXPECT_EQ(pin.mountType, mountCases.at(i).second) << "pin " << pin.name;
		EXPECT_EQ(pin.type, EdaDataFile::PackageRecord::PinRecord::Type::ThroughHole);
		EXPECT_EQ(pin.electricalType, EdaDataFile::PackageRecord::PinRecord::ElectricalType::Electrical);
		EXPECT_EQ(pin.id, std::numeric_limits<unsigned int>::max()); // no ID= field: sentinel
		EXPECT_EQ(pin.index, i); // finalized sequentially
	}
}

TEST(EdaDataFileTests, Parse_PinVariants_BlindSurfaceAndElectricalTypes)
{
	// spec p.113: type B -- Blind, S -- Surface; etype U -- Undefined,
	// M -- Electrical-Mechanical (parser maps it to NonElectrical)
	const auto dir = writeEdaData(
		"UNITS=MM\n"
		"PKG mixed 0.1 -1 -1 1 1\n"
		"PIN 1 T -0.2 0.1 0 E S ID=1034\n"
		"RC -0.2125 -0.145 0.025 0.069\n"
		"PIN 3 B -0.1 0 0 U D ID=1038\n"
		"CR 0 0 0.5\n"
		"PIN 5 S 0.1 0 0 M T ID=1041\n"
		"SQ 0.1 0 0.25\n", "pin_variants");

	EdaDataFile eda;
	ASSERT_TRUE(eda.Parse(dir));
	ASSERT_EQ(eda.GetPackageRecords().size(), 1U);

	const auto& package = *eda.GetPackageRecords().at(0);
	ASSERT_EQ(package.m_pinRecords.size(), 3U);

	const auto& throughHole = *package.m_pinRecords.at(0);
	EXPECT_EQ(throughHole.name, "1");
	EXPECT_EQ(throughHole.type, EdaDataFile::PackageRecord::PinRecord::Type::ThroughHole);
	EXPECT_EQ(throughHole.electricalType, EdaDataFile::PackageRecord::PinRecord::ElectricalType::Electrical);
	EXPECT_EQ(throughHole.mountType, EdaDataFile::PackageRecord::PinRecord::MountType::Smt);
	EXPECT_EQ(throughHole.id, 1034U);
	EXPECT_EQ(throughHole.index, 0U);
	// outline attached to the preceding pin
	ASSERT_EQ(throughHole.m_outlineRecords.size(), 1U);
	EXPECT_EQ(throughHole.m_outlineRecords.at(0)->type, EdaDataFile::PackageRecord::OutlineRecord::Type::Rectangle);

	const auto& blind = *package.m_pinRecords.at(1);
	EXPECT_EQ(blind.type, EdaDataFile::PackageRecord::PinRecord::Type::Blind);
	EXPECT_EQ(blind.electricalType, EdaDataFile::PackageRecord::PinRecord::ElectricalType::Undefined);
	EXPECT_EQ(blind.mountType, EdaDataFile::PackageRecord::PinRecord::MountType::RecommendedSmtPad);
	EXPECT_EQ(blind.id, 1038U);
	ASSERT_EQ(blind.m_outlineRecords.size(), 1U);
	EXPECT_EQ(blind.m_outlineRecords.at(0)->type, EdaDataFile::PackageRecord::OutlineRecord::Type::Circle);
	EXPECT_DOUBLE_EQ(blind.m_outlineRecords.at(0)->radius, 0.5);

	const auto& surface = *package.m_pinRecords.at(2);
	EXPECT_EQ(surface.type, EdaDataFile::PackageRecord::PinRecord::Type::Surface);
	EXPECT_EQ(surface.electricalType, EdaDataFile::PackageRecord::PinRecord::ElectricalType::NonElectrical);
	EXPECT_EQ(surface.mountType, EdaDataFile::PackageRecord::PinRecord::MountType::MT_ThroughHole);
	ASSERT_EQ(surface.m_outlineRecords.size(), 1U);
	EXPECT_EQ(surface.m_outlineRecords.at(0)->type, EdaDataFile::PackageRecord::OutlineRecord::Type::Square);
	EXPECT_DOUBLE_EQ(surface.m_outlineRecords.at(0)->halfSide, 0.25);
}

TEST(EdaDataFileTests, Parse_MalformedPinValues_ThrowParseError)
{
	const std::vector<std::pair<std::string, std::string>> malformed = {
		{ "unknown_pin_type", "PKG p 0.1 0 0 1 1\nPIN 1 X 0 0 0 E S\n" },
		{ "unknown_electrical_type", "PKG p 0.1 0 0 1 1\nPIN 1 T 0 0 0 N S\n" },
		{ "unknown_mount_type", "PKG p 0.1 0 0 1 1\nPIN 1 T 0 0 0 E Z\n" },
		{ "missing_coordinates", "PKG p 0.1 0 0 1 1\nPIN 1 T 0\n" },
	};

	int caseIndex = 0;
	for (const auto& [tag, pinLines] : malformed)
	{
		const auto dir = writeEdaData(
			"UNITS=MM\n" + pinLines, "bad_pin_" + std::to_string(caseIndex++));

		EdaDataFile eda;
		EXPECT_THROW(eda.Parse(dir), Odb::Lib::FileModel::parse_error)
			<< "case " << tag << " was not rejected";
	}
}

TEST(EdaDataFileTests, Parse_PackageContourOutline_IslandWithArcAndHole)
{
	// spec p.114: CT ... OB/OS/OC ... OE ... CE -- a contour consists of
	// one or more polygons (same structure as a features-file surface)
	const auto dir = writeEdaData(
		"UNITS=MM\n"
		"PKG contoured 0.1 0 0 2 2\n"
		"CT\n"
		"OB 0 0 I\n"
		"OS 1 0\n"
		"OC 2 0 1 0 n\n"
		"OS 2 1\n"
		"OE\n"
		"OB 0.5 0.5 H\n"
		"OS 0.75 0.5\n"
		"OE\n"
		"CE\n", "package_contour");

	EdaDataFile eda;
	ASSERT_TRUE(eda.Parse(dir));
	ASSERT_EQ(eda.GetPackageRecords().size(), 1U);

	const auto& package = *eda.GetPackageRecords().at(0);
	ASSERT_EQ(package.m_outlineRecords.size(), 1U);

	const auto& contour = *package.m_outlineRecords.at(0);
	EXPECT_EQ(contour.type, EdaDataFile::PackageRecord::OutlineRecord::Type::Contour);
	ASSERT_EQ(contour.m_contourPolygons.size(), 2U);

	const auto& island = *contour.m_contourPolygons.at(0);
	EXPECT_EQ(island.type, ContourPolygon::Type::Island);
	EXPECT_DOUBLE_EQ(island.xStart, 0.0);
	EXPECT_DOUBLE_EQ(island.yStart, 0.0);
	ASSERT_EQ(island.m_polygonParts.size(), 3U);
	const auto& arc = *island.m_polygonParts.at(1);
	EXPECT_EQ(arc.type, ContourPolygon::PolygonPart::Type::Arc);
	EXPECT_DOUBLE_EQ(arc.endX, 2.0);
	EXPECT_DOUBLE_EQ(arc.endY, 0.0);
	EXPECT_DOUBLE_EQ(arc.xCenter, 1.0);
	EXPECT_DOUBLE_EQ(arc.yCenter, 0.0);
	EXPECT_FALSE(arc.isClockwise);

	const auto& hole = *contour.m_contourPolygons.at(1);
	EXPECT_EQ(hole.type, ContourPolygon::Type::Hole);
	ASSERT_EQ(hole.m_polygonParts.size(), 1U);
}

TEST(EdaDataFileTests, Parse_PackageScopedProperty_AttachesToPackage)
{
	const auto dir = writeEdaData(
		"UNITS=MM\n"
		"PKG p 0.1 0 0 1 1\n"
		"PRP PKG_PROP 'v'\n"
		"PIN 1 T 0 0 0 E S\n", "package_scoped_prp");

	EdaDataFile eda;
	ASSERT_TRUE(eda.Parse(dir));

	EXPECT_EQ(eda.GetPropertyRecords().size(), 0U);
	ASSERT_EQ(eda.GetPackageRecords().size(), 1U);
	const auto& package = *eda.GetPackageRecords().at(0);
	ASSERT_EQ(package.m_propertyRecords.size(), 1U);
	EXPECT_EQ(package.m_propertyRecords.at(0)->name, "PKG_PROP");
	EXPECT_EQ(package.m_propertyRecords.at(0)->value, "v");
	// EOF finalization flushes the open package and its open pin
	ASSERT_EQ(package.m_pinRecords.size(), 1U);
	EXPECT_EQ(package.m_pinRecords.at(0)->name, "1");
	EXPECT_EQ(package.m_pinRecords.at(0)->index, 0U);
}

TEST(EdaDataFileTests, Parse_FeatureGroupRecord_TypePropertyAndFids)
{
	// spec p.114: "FGR <type>, where <type> is the string TEXT -- the only
	// value allowed"; each FGR line is followed by zero or more FID records
	const auto dir = writeEdaData(
		"UNITS=MM\n"
		"FGR TEXT\n"
		"PRP string '030'\n"
		"FID C 14 11018\n"
		"FID C 14 11019\n", "feature_group");

	EdaDataFile eda;
	ASSERT_TRUE(eda.Parse(dir));

	EXPECT_EQ(eda.GetPropertyRecords().size(), 0U); // scoped to the FGR
	const auto& groups = eda.GetFeatureGroupRecords();
	ASSERT_EQ(groups.size(), 1U);

	const auto& group = *groups.at(0);
	EXPECT_EQ(group.type, "TEXT");
	ASSERT_EQ(group.m_propertyRecords.size(), 1U);
	EXPECT_EQ(group.m_propertyRecords.at(0)->name, "string");
	EXPECT_EQ(group.m_propertyRecords.at(0)->value, "030");
	ASSERT_EQ(group.m_featureIdRecords.size(), 2U);
	EXPECT_EQ(group.m_featureIdRecords.at(1)->featureNumber, 11019U);
}

TEST(EdaDataFileTests, Parse_AttributeNameValueRecords_AcceptedWithCommentPrefix)
{
	// backward compatibility: attribute lines may carry the comment
	// character prefix (spec example shows "#@0 .critical_net").
	// Note the net record here carries an attributes-only trailer
	// ("NET n ;0,1=1"): after the NET branch strips the name's ';',
	// ParseAttributeLookupTable skips everything before the first ';',
	// so the assignments never reach the lookup table -- DOCUMENTED
	// DEFECT (see Parse_NetRecord_SpecAttributesForm_LosesAttributesAndId).
	const auto dir = writeEdaData(
		"UNITS=MM\n"
		"#@0 .critical_net\n"
		"#@1 .net_type\n"
		"#&1 clocks\n"
		"NET n ;0,1=1\n", "attr_tables");

	EdaDataFile eda;
	ASSERT_TRUE(eda.Parse(dir));
	ASSERT_EQ(eda.GetNetRecords().size(), 1U);
	const auto attributes = snapshotAttributes(*eda.GetNetRecords().at(0));
	EXPECT_TRUE(attributes.empty()); // defect: assignments lost
}

TEST(EdaDataFileTests, Parse_NetNameWithoutSemicolon_ToleratedWithTruncatedName)
{
	// documented tolerance: "NET <name>" without any ';' still parses --
	// getline returns the rest of the line as the net name (the spec
	// grammar on p.110 requires the ';' separators). Only a completely
	// name-less "NET" row is a parse error.
	const auto dir = writeEdaData(
		"UNITS=MM\n"
		"NET broken\n", "net_no_semicolon");

	EdaDataFile eda;
	ASSERT_TRUE(eda.Parse(dir));
	ASSERT_EQ(eda.GetNetRecords().size(), 1U);
	EXPECT_EQ(eda.GetNetRecords().at(0)->name, "broken");

	const auto dirBare = writeEdaData(
		"UNITS=MM\n"
		"NET\n", "net_alone");

	EdaDataFile edaBare;
	EXPECT_THROW(edaBare.Parse(dirBare), Odb::Lib::FileModel::parse_error);
}

TEST(EdaDataFileTests, Parse_PackageWithMissingBounds_ThrowsParseError)
{
	// PKG requires six fields plus the ';' trailer before the attribute
	// section (spec p.112 grammar)
	const auto dir = writeEdaData(
		"UNITS=MM\n"
		"PKG p 0.1 0 0 1\n", "pkg_missing_bounds");

	EdaDataFile eda;
	EXPECT_THROW(eda.Parse(dir), Odb::Lib::FileModel::parse_error);
}

TEST(EdaDataFileTests, Parse_PackageWithNonNumericBounds_ThrowsException)
{
	// stream-extracted bounds (xmin..xmax) fail with parse_error...
	const auto dirStreamPath = writeEdaData(
		"UNITS=MM\n"
		"PKG p 0.1 abc 0 1 1\n", "pkg_bad_bounds_stream");

	EdaDataFile edaStreamPath;
	EXPECT_THROW(edaStreamPath.Parse(dirStreamPath), Odb::Lib::FileModel::parse_error);

	// ...while ymax is read with std::stof on the pre-';' token, so a
	// non-numeric ymax surfaces as std::exception via the generic catch
	// clause ('throw e' slices the concrete type)
	const auto dirStofPath = writeEdaData(
		"UNITS=MM\n"
		"PKG p 0.1 0 0 1 abc\n", "pkg_bad_bounds_stof");

	EdaDataFile edaStofPath;
	EXPECT_THROW(edaStofPath.Parse(dirStofPath), std::exception);
}

TEST(EdaDataFileTests, Parse_OrphanOutlineRecords_ThrowParseError)
{
	const std::vector<std::pair<std::string, std::string>> orphans = {
		{ "rc_without_package", "RC 0 0 1 1\n" },
		{ "cr_without_package", "CR 0 0 1\n" },
		{ "sq_without_package", "SQ 0 0 1\n" },
		{ "ce_without_contour", "CE\n" },
		{ "oe_without_contour", "OE\n" },
		{ "os_without_ob", "OS 1 1\n" },
	};

	int caseIndex = 0;
	for (const auto& [tag, line] : orphans)
	{
		const auto dir = writeEdaData(
			"UNITS=MM\n" + line, "orphan_outline_" + std::to_string(caseIndex++));

		EdaDataFile eda;
		EXPECT_THROW(eda.Parse(dir), Odb::Lib::FileModel::parse_error)
			<< "case " << tag << " was not rejected";
	}
}

TEST(EdaDataFileTests, Parse_MalformedHeader_ThrowsParseError)
{
	const auto dir = writeEdaData("HDR\n", "hdr_alone");

	EdaDataFile eda;
	EXPECT_THROW(eda.Parse(dir), Odb::Lib::FileModel::parse_error);
}

TEST(EdaDataFileTests, Parse_UnitsWithoutValue_ThrowsParseError)
{
	const auto dir = writeEdaData("UNITS\n", "units_alone");

	EdaDataFile eda;
	EXPECT_THROW(eda.Parse(dir), Odb::Lib::FileModel::parse_error);
}

TEST(EdaDataFileTests, Parse_UnrecognizedLine_WarnedAndSkipped)
{
	// documented tolerance: unknown lines log a warning and are skipped
	const auto dir = writeEdaData(
		"UNITS=MM\n"
		"WHATEVER 1 2 3\n"
		"NET n\n", "unrecognized_line");

	EdaDataFile eda;
	ASSERT_TRUE(eda.Parse(dir));
	ASSERT_EQ(eda.GetNetRecords().size(), 1U);
}

TEST(EdaDataFileTests, Parse_CommentsAndBlankLines_Skipped)
{
	const auto dir = writeEdaData(
		"# leading comment\n"
		"\n"
		"UNITS=MM\n"
		"# NET fake inside comment\n"
		"NET real\n"
		"\n", "comments");

	EdaDataFile eda;
	ASSERT_TRUE(eda.Parse(dir));
	ASSERT_EQ(eda.GetNetRecords().size(), 1U);
	EXPECT_EQ(eda.GetNetRecords().at(0)->name, "real");
}

TEST(EdaDataFileTests, Parse_MissingDataFile_ThrowsInvalidOdbError)
{
	const auto dir = fs::temp_directory_path() / "odbdesign_edadata_tests" / "empty_dir";
	fs::create_directories(dir);

	// the parser throws invalid_odb_error ("eda/data file does not
	// exist"), but the generic `catch (std::exception&) { ...; throw e; }`
	// handler slices it to a bare std::exception -- the concrete type and
	// message are lost. Pinned as the observed contract.
	EdaDataFile eda;
	EXPECT_THROW(eda.Parse(dir), std::exception);
}

TEST(EdaDataFileTests, Parse_NameLookupMaps_EmptyByContract)
{
	// documented contract: Parse never fills netRecordsByName /
	// packageRecordsByName (those lookups are assembled at higher layers);
	// parsing a full file must leave them empty
	const auto dir = writeEdaData(
		"UNITS=MM\n"
		"NET vcc\n"
		"PKG p 0.1 0 0 1 1\n"
		"PIN 1 T 0 0 0 E S\n", "name_maps");

	EdaDataFile eda;
	ASSERT_TRUE(eda.Parse(dir));
	EXPECT_TRUE(eda.GetNetRecordsByName().empty());
	EXPECT_TRUE(eda.GetPackageRecordsByName().empty());
}

TEST(EdaDataFileTests, ProtobufRoundTrip_DeepEquivalenceOverNetsSubnetsPackagesAndGroups)
{
	const auto dir = writeEdaData(
		"UNITS=MM\n"
		"HDR Mentor Boardstation database\n"
		"LYR sst drill\n"
		"PRP TOP_PROP 't'\n"
		"#@0 .critical_net\n"
		"NET /D_CL_TX_CLK ;0,2=1;ID=1030\n"
		"PRP NET_PROP 'n'\n"
		"SNT TOP T 16 0\n"
		"FID C 2 33\n"
		"SNT VIA\n"
		"FID H 12 5\n"
		"SNT PLN O E 2.5\n"
		"FID L 3 4\n"
		"NET gnd\n"
		"PKG *PDXC-L10 0.1 -0.25 -0.145 0.25 0.145 ;3=2;ID=1044\n"
		"RC -0.25 -0.1 0.5 0.2\n"
		"PRP PKG_PROP 'p'\n"
		"PIN 1 T -0.2 -0.1105 0 E S ID=1034\n"
		"CR 0 0 0.5\n"
		"CT\n"
		"OB 0 0 I\n"
		"OS 1 0\n"
		"OE\n"
		"CE\n"
		"FGR TEXT\n"
		"PRP string '030'\n"
		"FID C 14 11018\n", "protobuf_roundtrip");

	using ProtobufEdaDataFile = Odb::Lib::Protobuf::EdaDataFile;

	EdaDataFile original;
	ASSERT_TRUE(original.Parse(dir));
	auto originalMessage = original.to_protobuf();
	ASSERT_NE(originalMessage, nullptr);

	EdaDataFile roundTripped;
	roundTripped.from_protobuf(*originalMessage);
	auto roundTrippedMessage = roundTripped.to_protobuf();

	EXPECT_EQ(roundTrippedMessage->units(), "MM");
	EXPECT_EQ(roundTrippedMessage->source(), "Mentor Boardstation database");
	ASSERT_EQ(roundTrippedMessage->layernames_size(), 2);
	EXPECT_EQ(roundTrippedMessage->layernames(1), "drill");
	ASSERT_EQ(roundTrippedMessage->attributenames_size(), 1);
	EXPECT_EQ(roundTrippedMessage->attributenames(0), ".critical_net");
	EXPECT_EQ(roundTrippedMessage->propertyrecords_size(), 1);
	EXPECT_EQ(roundTrippedMessage->propertyrecords(0).name(), "TOP_PROP");

	// nets
	ASSERT_EQ(roundTrippedMessage->netrecords_size(), 2);
	{
		const auto& net0 = roundTrippedMessage->netrecords(0);
		EXPECT_EQ(net0.name(), "/D_CL_TX_CLK");
		EXPECT_EQ(net0.index(), 0U);
		// documented defect (see Parse_NetRecord tests): the uid survives
		// only as a fake "ID" attribute, never as the typed id field
		EXPECT_FALSE(net0.has_id());
		EXPECT_EQ(net0.propertyrecords_size(), 1);
		EXPECT_EQ(net0.propertyrecords(0).value(), "n");
		ASSERT_EQ(net0.subnetrecords_size(), 3);
		EXPECT_EQ(net0.subnetrecords(0).type(), ProtobufEdaDataFile::NetRecord::SubnetRecord::TOEPRINT);
		EXPECT_EQ(net0.subnetrecords(0).side(), Odb::Lib::Protobuf::BoardSide::Top);
		EXPECT_EQ(net0.subnetrecords(0).componentnumber(), 16U);
		EXPECT_EQ(net0.subnetrecords(0).toeprintnumber(), 0U);
		EXPECT_EQ(net0.subnetrecords(0).featureidrecords_size(), 1);
		EXPECT_EQ(net0.subnetrecords(1).type(), ProtobufEdaDataFile::NetRecord::SubnetRecord::VIA);
		EXPECT_EQ(net0.subnetrecords(2).type(), ProtobufEdaDataFile::NetRecord::SubnetRecord::PLANE);
		EXPECT_EQ(net0.subnetrecords(2).filltype(), ProtobufEdaDataFile::NetRecord::SubnetRecord::OUTLINE);
		EXPECT_EQ(net0.subnetrecords(2).cutouttype(), ProtobufEdaDataFile::NetRecord::SubnetRecord::EXACT);
		EXPECT_DOUBLE_EQ(net0.subnetrecords(2).fillsize(), 2.5);
		EXPECT_EQ(net0.subnetrecords(2).featureidrecords(0).type(),
			ProtobufEdaDataFile::FeatureIdRecord::LAMINATE);

		// attribute lookup table (protobuf map: iterate)
		std::map<std::string, std::string> netAttributes;
		for (const auto& kv : net0.attributelookuptable())
		{
			netAttributes[kv.first] = kv.second;
		}
		ASSERT_EQ(netAttributes.size(), 1U);
		EXPECT_EQ(netAttributes.at("ID"), "1030");

		const auto& net1 = roundTrippedMessage->netrecords(1);
		EXPECT_EQ(net1.name(), "gnd");
		EXPECT_FALSE(net1.has_id()); // absent optional stays absent
	}

	// packages
	ASSERT_EQ(roundTrippedMessage->packagerecords_size(), 1);
	{
		const auto& package = roundTrippedMessage->packagerecords(0);
		EXPECT_EQ(package.name(), "*PDXC-L10");
		EXPECT_DOUBLE_EQ(package.pitch(), 0.1);
		EXPECT_FALSE(package.has_id()); // same trailer defect as NET records
		EXPECT_EQ(package.propertyrecords_size(), 1);
		EXPECT_EQ(package.propertyrecords(0).value(), "p");
		ASSERT_EQ(package.pinrecords_size(), 1);
		const auto& pin = package.pinrecords(0);
		EXPECT_EQ(pin.name(), "1");
		EXPECT_EQ(pin.type(), ProtobufEdaDataFile::PackageRecord::PinRecord::ThroughHole);
		EXPECT_EQ(pin.electricaltype(), ProtobufEdaDataFile::PackageRecord::PinRecord::Electrical);
		EXPECT_EQ(pin.mounttype(), ProtobufEdaDataFile::PackageRecord::PinRecord::Smt);
		EXPECT_EQ(pin.id(), 1034U);
		EXPECT_EQ(pin.index(), 0U);
		// note: the protobuf PinRecord message carries no outline records
		// (only the C++ object does); the package-level RC survives
		ASSERT_EQ(package.outlinerecords_size(), 1);
		EXPECT_EQ(package.outlinerecords(0).type(), ProtobufEdaDataFile::PackageRecord::OutlineRecord::Rectangle);
	}

	// feature groups
	ASSERT_EQ(roundTrippedMessage->featuregrouprecords_size(), 1);
	{
		const auto& group = roundTrippedMessage->featuregrouprecords(0);
		EXPECT_EQ(group.type(), "TEXT");
		EXPECT_EQ(group.propertyrecords_size(), 1);
		EXPECT_EQ(group.propertyrecords(0).value(), "030");
		ASSERT_EQ(group.featureidrecords_size(), 1);
		EXPECT_EQ(group.featureidrecords(0).featurenumber(), 11018U);
	}

	// name-keyed maps stay empty on both sides (parse never fills them)
	EXPECT_EQ(roundTrippedMessage->netrecordsbyname_size(), 0);
	EXPECT_EQ(roundTrippedMessage->packagerecordsbyname_size(), 0);
}

TEST(EdaDataFileTests, ProtobufRoundTrip_AbsentOptionalNetId_StaysAbsent)
{
	const auto dir = writeEdaData(
		"UNITS=MM\n"
		"NET plain\n", "optional_net_id");

	EdaDataFile original;
	ASSERT_TRUE(original.Parse(dir));
	auto message = original.to_protobuf();
	ASSERT_EQ(message->netrecords_size(), 1);
	EXPECT_FALSE(message->netrecords(0).has_id());

	EdaDataFile roundTripped;
	roundTripped.from_protobuf(*message);
	ASSERT_EQ(roundTripped.GetNetRecords().size(), 1U);
	EXPECT_FALSE(roundTripped.GetNetRecords().at(0)->id.has_value());
	EXPECT_EQ(roundTripped.GetNetRecords().at(0)->name, "plain");
	EXPECT_EQ(roundTripped.GetNetRecords().at(0)->index, 0U);
}

TEST(EdaDataFileTests, RealDesigns_NetRecordIdTrailer_LandsInAttributeMapNotIdField)
{
	// REAL-CORPUS pin of the NET trailer defect: every NET row in the
	// local corpora carries the ";;ID=<uid>" trailer, and after a real
	// parse every net has id unset with "ID"=<uid> sitting in its
	// attribute lookup table. When the trailer parsing is fixed this test
	// must flip together with the synthetic pins.
	const char* testDataDir = std::getenv("ODB_TEST_DATA_DIR");
	if (testDataDir == nullptr || testDataDir[0] == '\0')
	{
		GTEST_SKIP() << "ODB_TEST_DATA_DIR not set";
	}

	const auto edaDir = fs::path(testDataDir) / "designodb_rigidflex" /
		"designodb_rigidflex" / "steps" / "cellular_flip-phone" / "eda";
	if (!fs::exists(edaDir))
	{
		GTEST_SKIP() << "designodb_rigidflex eda dir not in test data dir";
	}

	EdaDataFile eda;
	ASSERT_TRUE(eda.Parse(edaDir));

	const auto& nets = eda.GetNetRecords();
	ASSERT_GT(nets.size(), 0U);

	unsigned withTypedId = 0;
	unsigned withIdAttribute = 0;
	for (const auto& net : nets)
	{
		if (net->id.has_value())
		{
			++withTypedId;
		}
		if (net->GetAttributeLookupTable().find("ID") != net->GetAttributeLookupTable().end())
		{
			++withIdAttribute;
		}
	}
	EXPECT_EQ(withTypedId, 0U);
	EXPECT_EQ(withIdAttribute, nets.size());
}
