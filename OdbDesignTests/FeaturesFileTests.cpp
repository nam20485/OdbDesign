// Specification-driven tests for FeaturesFile (<layer_name>/features).
//
// ODB++ spec 8.1 U4, "Layer Entities / <layer_name>/features (Graphic
// Features)", pp. 164-176: the file consists of the UNITS directive
// (required), an optional ID, an optional "F <n>" feature count, the
// "$" symbol-name section, the "@"/"&" attribute lookup tables, and the
// feature records L (line, p.169), P (pad, p.170), A (arc, p.172),
// T (text, p.173), B (barcode, p.176) and S/OB/OS/OC/OE/SE surfaces.
//
// Every test writes a synthetic features file into its own temp directory,
// parses it through the real FeaturesFile::Parse, and asserts values
// computed independently from the spec grammar. Barcode parsing is a stub
// in the parser (no fields are read); its no-op contract is pinned
// explicitly rather than faked. Per plan §5.8, Save() is not exercised
// (it is a no-op stub).
//
// Protobuf maps are only ever iterated, never find()/at()'d (issue #599).

#include "FileModel/Design/FeaturesFile.h"
#include "FileModel/Design/ContourPolygon.h"
#include "FileModel/parse_error.h"
#include "FileModel/invalid_odb_error.h"

#include <gtest/gtest.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace Odb::Lib::FileModel::Design;
using ProtobufFeaturesFile = Odb::Lib::Protobuf::FeaturesFile;

namespace
{
	fs::path writeFeatures(const std::string& contents, const std::string& tag, const std::string& filename = "features")
	{
		auto dir = fs::temp_directory_path() / "odbdesign_featuresfile_tests" / tag;
		fs::create_directories(dir);
		std::ofstream out(dir / filename, std::ios::trunc);
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

	// symbolNamesByName is a std::map (not protobuf), but the protobuf map
	// sibling is snapshot-by-iteration for uniformity with the #599 rule.
	std::map<std::string, const Odb::Lib::Protobuf::SymbolName*> snapshotSymbols(const ProtobufFeaturesFile& message)
	{
		std::map<std::string, const Odb::Lib::Protobuf::SymbolName*> symbols;
		for (const auto& kv : message.symbolnamesbyname())
		{
			symbols[kv.first] = &kv.second;
		}
		return symbols;
	}
}

TEST(FeaturesFileTests, Parse_LineRecord_ExtractsAllFieldsAndAttributes)
{
	// grammar (spec p.169): L <xs> <ys> <xe> <ye> <sym_num> <polarity> <dcode>;<atr>=<value>,...;ID=<id>
	const auto dir = writeFeatures(
		"UNITS=MM\n"
		"L 3.834 -1.16 3.86728 -1.16485 2 P 5 ;1,3=0;ID=4321\n", "line_record");

	FeaturesFile features;
	ASSERT_TRUE(features.Parse(dir));
	ASSERT_EQ(features.GetFeatureRecords().size(), 1U);

	const auto& line = *features.GetFeatureRecords().at(0);
	EXPECT_EQ(line.type, FeaturesFile::FeatureRecord::Type::Line);
	EXPECT_DOUBLE_EQ(line.xs, 3.834);
	EXPECT_DOUBLE_EQ(line.ys, -1.16);
	EXPECT_DOUBLE_EQ(line.xe, 3.86728);
	EXPECT_DOUBLE_EQ(line.ye, -1.16485);
	EXPECT_EQ(line.sym_num, 2);
	EXPECT_EQ(line.polarity, Odb::Lib::Polarity::Positive);
	EXPECT_EQ(line.dcode, 5);
	ASSERT_TRUE(line.id.has_value());
	EXPECT_EQ(*line.id, 4321U);

	const auto attributes = snapshotAttributes(line);
	EXPECT_EQ(attributes.size(), 2U);
	EXPECT_EQ(attributes.at("1"), "");
	EXPECT_EQ(attributes.at("3"), "0");
}

TEST(FeaturesFileTests, Parse_NegativePolarityLine_Assigned)
{
	// spec p.169: polarity "N -- Negative"
	const auto dir = writeFeatures(
		"UNITS=MM\n"
		"L 0 0 1 1 0 N 0\n", "negative_line");

	FeaturesFile features;
	ASSERT_TRUE(features.Parse(dir));
	ASSERT_EQ(features.GetFeatureRecords().size(), 1U);
	EXPECT_EQ(features.GetFeatureRecords().at(0)->polarity, Odb::Lib::Polarity::Negative);
	EXPECT_FALSE(features.GetFeatureRecords().at(0)->id.has_value());
}

TEST(FeaturesFileTests, Parse_PadRecord_ExtractsSymbolDcodeAndOrientation)
{
	// grammar (spec p.170): P <x> <y> <apt_def> <polarity> <dcode> <orient_def>;...;ID=<id>
	// spec example: "P 1.0 2.0 0 P 4 1;;ID=3948"
	const auto dir = writeFeatures(
		"UNITS=INCH\n"
		"P 1.0 2.0 16 P 4 3;3=2,4=0;ID=123456\n", "pad_record");

	FeaturesFile features;
	ASSERT_TRUE(features.Parse(dir));
	ASSERT_EQ(features.GetFeatureRecords().size(), 1U);

	const auto& pad = *features.GetFeatureRecords().at(0);
	EXPECT_EQ(pad.type, FeaturesFile::FeatureRecord::Type::Pad);
	EXPECT_DOUBLE_EQ(pad.x, 1.0);
	EXPECT_DOUBLE_EQ(pad.y, 2.0);
	EXPECT_EQ(pad.apt_def_symbol_num, 16);
	EXPECT_EQ(pad.sym_num, 16); // contract: sym_num mirrors a direct symbol ref
	EXPECT_EQ(pad.polarity, Odb::Lib::Polarity::Positive);
	EXPECT_EQ(pad.dcode, 4);
	EXPECT_EQ(pad.orient_def, 3);
	EXPECT_DOUBLE_EQ(pad.orient_def_rotation, 0.0); // legacy orientations carry no rotation field
	ASSERT_TRUE(pad.id.has_value());
	EXPECT_EQ(*pad.id, 123456U);

	const auto attributes = snapshotAttributes(pad);
	EXPECT_EQ(attributes.at("3"), "2");
	EXPECT_EQ(attributes.at("4"), "0");
}

TEST(FeaturesFileTests, Parse_PadResizedSymbol_CarriesResizeFactor)
{
	// The parser reads a resized pad as a TWO-number form: "-1 <resize>".
	// The spec (p.170) defines a THREE-number form "-1 <sym_num>
	// <resize_factor>"; the next test pins how that spec form is rejected.
	const auto dir = writeFeatures(
		"UNITS=INCH\n"
		"P 1.0 2.0 -1 0.02 P 4 1;;ID=7421\n", "pad_resize");

	FeaturesFile features;
	ASSERT_TRUE(features.Parse(dir));
	ASSERT_EQ(features.GetFeatureRecords().size(), 1U);

	const auto& pad = *features.GetFeatureRecords().at(0);
	EXPECT_EQ(pad.apt_def_symbol_num, -1);
	EXPECT_DOUBLE_EQ(pad.apt_def_resize_factor, 0.02);
	// documented contract: with apt_def_symbol_num == -1 no symbol index is
	// recorded; sym_num stays unset (-1)
	EXPECT_EQ(pad.sym_num, -1);
	ASSERT_TRUE(pad.id.has_value());
	EXPECT_EQ(*pad.id, 7421U);
}

TEST(FeaturesFileTests, Parse_PadSpecThreeNumberResizeForm_RejectedWithParseError)
{
	// DOCUMENTED DIVERGENCE: the spec example (p.172)
	// "P 1.0 2.0 -1 0 0.02 P 4 1;;ID=7421" uses the three-number resize
	// form. The parser consumes the middle <sym_num> as the resize factor
	// and then finds '0' where the polarity character belongs, so the
	// spec-conformant record is a parse error. Pinned so a fix must
	// consciously adopt the spec grammar.
	const auto dir = writeFeatures(
		"UNITS=INCH\n"
		"P 1.0 2.0 -1 0 0.02 P 4 1;;ID=7421\n", "pad_resize_spec_form");

	FeaturesFile features;
	EXPECT_THROW(features.Parse(dir), Odb::Lib::FileModel::parse_error);
}

TEST(FeaturesFileTests, Parse_PadOrientationEight_Nine_LegacyAndDocumentedGap)
{
	// spec p.170: orient_def "8|9" is a two-number definition, the second
	// number being the rotation. The TEXT branch reads that rotation; the
	// PAD branch does not, so a spec-conformant "P ... 8 30.0;;ID=..." row
	// parses with orient_def=8 while the rotation is consumed as part of
	// the attribute section (documented divergence; see PR notes).
	const auto dir = writeFeatures(
		"UNITS=INCH\n"
		"P 1.0 2.0 0 P 4 8 30.0;;ID=7205\n", "pad_orientation8");

	FeaturesFile features;
	ASSERT_TRUE(features.Parse(dir));
	ASSERT_EQ(features.GetFeatureRecords().size(), 1U);

	const auto& pad = *features.GetFeatureRecords().at(0);
	EXPECT_EQ(pad.orient_def, 8);
	EXPECT_DOUBLE_EQ(pad.orient_def_rotation, 0.0); // current behavior: rotation not read for pads
	ASSERT_TRUE(pad.id.has_value());
	EXPECT_EQ(*pad.id, 7205U);
}

TEST(FeaturesFileTests, Parse_TextRecord_ExtractsAllFieldsIncludingQuotedText)
{
	// grammar (spec p.173): T <x> <y> <font> <polarity> <orient_def>
	// [rotation] <xsize> <ysize> <width_factor> <text> <version>;...;ID=<id>
	// spec dynamic-text example (p.175) uses orientation 8 with rotation.
	const auto dir = writeFeatures(
		"UNITS=MM\n"
		"T 11.890963 13.697185 standard P 8 30.0 5.08 5.08 2.0 '$$DATE-MMDDYY' 1;2=1;ID=777\n"
		"T 4.033375 6.377506 standard N 0 0.2 0.2 2.00000 'HELLO WORLD' 0\n", "text_record");

	FeaturesFile features;
	ASSERT_TRUE(features.Parse(dir));
	ASSERT_EQ(features.GetFeatureRecords().size(), 2U);

	const auto& rotated = *features.GetFeatureRecords().at(0);
	EXPECT_EQ(rotated.type, FeaturesFile::FeatureRecord::Type::Text);
	EXPECT_DOUBLE_EQ(rotated.x, 11.890963);
	EXPECT_DOUBLE_EQ(rotated.y, 13.697185);
	EXPECT_EQ(rotated.font, "standard");
	EXPECT_EQ(rotated.polarity, Odb::Lib::Polarity::Positive);
	EXPECT_EQ(rotated.orient_def, 8);
	EXPECT_DOUBLE_EQ(rotated.orient_def_rotation, 30.0);
	EXPECT_DOUBLE_EQ(rotated.xsize, 5.08);
	EXPECT_DOUBLE_EQ(rotated.ysize, 5.08);
	EXPECT_DOUBLE_EQ(rotated.width_factor, 2.0);
	EXPECT_EQ(rotated.text, "$$DATE-MMDDYY");
	EXPECT_EQ(rotated.version, 1);
	ASSERT_TRUE(rotated.id.has_value());
	EXPECT_EQ(*rotated.id, 777U);
	EXPECT_EQ(rotated.dcode, 0);

	const auto& simple = *features.GetFeatureRecords().at(1);
	EXPECT_EQ(simple.polarity, Odb::Lib::Polarity::Negative);
	EXPECT_EQ(simple.orient_def, 0);
	EXPECT_DOUBLE_EQ(simple.orient_def_rotation, 0.0);
	EXPECT_EQ(simple.text, "HELLO WORLD"); // quoted text keeps inner spaces
	EXPECT_EQ(simple.version, 0);
	EXPECT_FALSE(simple.id.has_value());
}

TEST(FeaturesFileTests, Parse_ArcRecord_ExtractsCenterClockwiseAndPolarity)
{
	// grammar (spec p.172):
	// A <xs> <ys> <xe> <ye> <xc> <yc> <sym_num> <polarity> <dcode> <cw>
	// spec example: "A 1.49401033 2.53683297 0.8389936 2.48563819 1.17858858 2.35659203 2 P 0 Y"
	const auto dir = writeFeatures(
		"UNITS=MM\n"
		"A 1.49401033 2.53683297 0.8389936 2.48563819 1.17858858 2.35659203 2 P 0 Y\n"
		"A 0 0 1 1 0.5 0.5 1 N 7 N\n", "arc_record");

	FeaturesFile features;
	ASSERT_TRUE(features.Parse(dir));
	ASSERT_EQ(features.GetFeatureRecords().size(), 2U);

	const auto& clockwise = *features.GetFeatureRecords().at(0);
	EXPECT_EQ(clockwise.type, FeaturesFile::FeatureRecord::Type::Arc);
	EXPECT_DOUBLE_EQ(clockwise.xs, 1.49401033);
	EXPECT_DOUBLE_EQ(clockwise.ys, 2.53683297);
	EXPECT_DOUBLE_EQ(clockwise.xe, 0.8389936);
	EXPECT_DOUBLE_EQ(clockwise.ye, 2.48563819);
	EXPECT_DOUBLE_EQ(clockwise.xc, 1.17858858);
	EXPECT_DOUBLE_EQ(clockwise.yc, 2.35659203);
	EXPECT_EQ(clockwise.sym_num, 2);
	EXPECT_EQ(clockwise.polarity, Odb::Lib::Polarity::Positive);
	EXPECT_EQ(clockwise.dcode, 0);
	EXPECT_TRUE(clockwise.cw);

	const auto& counter = *features.GetFeatureRecords().at(1);
	EXPECT_EQ(counter.polarity, Odb::Lib::Polarity::Negative);
	EXPECT_EQ(counter.dcode, 7);
	EXPECT_FALSE(counter.cw);
}

TEST(FeaturesFileTests, Parse_SurfaceWithIslandAndHole_BuildsContourPolygons)
{
	// surface grammar (spec pp.164/176 and the features example, p.165):
	// S <polarity> <dcode>;<attrs>;ID=<id>  ...  OB <x> <y> I/H ... OS/OC ... OE ... SE
	// (the empty-attributes form is written ";;ID=" here -- see the next
	// test for how the single-';' form of the spec example is misrouted)
	const auto dir = writeFeatures(
		"UNITS=MM\n"
		"S P 0;;ID=123456\n"
		"OB -0.013 2.427 I\n"
		"OS -0.013 2.218\n"
		"OS -0.263 2.218\n"
		"OS -0.263 2.427\n"
		"OE\n"
		"OB 0.5 0.5 H\n"
		"OC 1.0 1.0 0.75 0.75 y\n"
		"OS 0.5 0.5\n"
		"OE\n"
		"SE\n", "surface_island_hole");

	FeaturesFile features;
	ASSERT_TRUE(features.Parse(dir));
	ASSERT_EQ(features.GetFeatureRecords().size(), 1U);

	const auto& surface = *features.GetFeatureRecords().at(0);
	EXPECT_EQ(surface.type, FeaturesFile::FeatureRecord::Type::Surface);
	EXPECT_EQ(surface.polarity, Odb::Lib::Polarity::Positive);
	EXPECT_EQ(surface.dcode, 0);
	ASSERT_TRUE(surface.id.has_value());
	EXPECT_EQ(*surface.id, 123456U);

	const auto& contours = surface.GetContourPolygons();
	ASSERT_EQ(contours.size(), 2U);

	const auto& island = *contours.at(0);
	EXPECT_EQ(island.type, ContourPolygon::Type::Island);
	EXPECT_DOUBLE_EQ(island.xStart, -0.013);
	EXPECT_DOUBLE_EQ(island.yStart, 2.427);
	ASSERT_EQ(island.m_polygonParts.size(), 3U);
	for (const auto& part : island.m_polygonParts)
	{
		EXPECT_EQ(part->type, ContourPolygon::PolygonPart::Type::Segment);
	}
	EXPECT_DOUBLE_EQ(island.m_polygonParts.at(1)->endX, -0.263);
	EXPECT_DOUBLE_EQ(island.m_polygonParts.at(1)->endY, 2.218);

	const auto& hole = *contours.at(1);
	EXPECT_EQ(hole.type, ContourPolygon::Type::Hole);
	EXPECT_DOUBLE_EQ(hole.xStart, 0.5);
	EXPECT_DOUBLE_EQ(hole.yStart, 0.5);
	ASSERT_EQ(hole.m_polygonParts.size(), 2U);
	const auto& arc = *hole.m_polygonParts.at(0);
	EXPECT_EQ(arc.type, ContourPolygon::PolygonPart::Type::Arc);
	EXPECT_DOUBLE_EQ(arc.endX, 1.0);
	EXPECT_DOUBLE_EQ(arc.endY, 1.0);
	EXPECT_DOUBLE_EQ(arc.xCenter, 0.75);
	EXPECT_DOUBLE_EQ(arc.yCenter, 0.75);
	EXPECT_TRUE(arc.isClockwise);
	EXPECT_EQ(hole.m_polygonParts.at(1)->type, ContourPolygon::PolygonPart::Type::Segment);
}

TEST(FeaturesFileTests, Parse_NegativeSurface_CarriesPolarityAndDcode)
{
	const auto dir = writeFeatures(
		"UNITS=MM\n"
		"S N 5\n"
		"OB 0 0 I\n"
		"OS 1 0\n"
		"OS 1 1\n"
		"OS 0 1\n"
		"OS 0 0\n"
		"OE\n"
		"SE\n", "surface_negative");

	FeaturesFile features;
	ASSERT_TRUE(features.Parse(dir));
	ASSERT_EQ(features.GetFeatureRecords().size(), 1U);
	const auto& surface = *features.GetFeatureRecords().at(0);
	EXPECT_EQ(surface.polarity, Odb::Lib::Polarity::Negative);
	EXPECT_EQ(surface.dcode, 5);
	EXPECT_FALSE(surface.id.has_value());
	EXPECT_EQ(surface.GetContourPolygons().size(), 1U);
}

TEST(FeaturesFileTests, Parse_SurfaceSingleSemicolonIdTrailer_MisroutedToAttributeMap)
{
	// DOCUMENTED DEFECT: the spec's own features example (p.165) writes a
	// no-attributes surface as "S P 0;ID=123456" (single ';'). The
	// attribute-lookup parser splits sections on ';' and expects the empty
	// attributes section to be explicit (";;ID="), so the single-';' form
	// consumes "ID=<uid>" as the ATTRIBUTES section: the record id stays
	// unset and a fake "ID" attribute is added to the lookup table.
	// Pinned exactly so a fix flips both assertions deliberately.
	const auto dir = writeFeatures(
		"UNITS=MM\n"
		"S P 0;ID=123456\n"
		"OB 0 0 I\n"
		"OS 1 1\n"
		"OE\n"
		"SE\n", "surface_single_semicolon_id");

	FeaturesFile features;
	ASSERT_TRUE(features.Parse(dir));
	ASSERT_EQ(features.GetFeatureRecords().size(), 1U);

	const auto& surface = *features.GetFeatureRecords().at(0);
	EXPECT_FALSE(surface.id.has_value()); // defect: uid never reaches the typed field
	const auto attributes = snapshotAttributes(surface);
	ASSERT_EQ(attributes.size(), 1U);
	EXPECT_EQ(attributes.at("ID"), "123456"); // defect: uid posing as an attribute
}

TEST(FeaturesFileTests, Parse_SurfaceWithoutSe_FinalizedAtEof)
{
	// the parser finalizes a still-open surface record when the file ends
	// (documented tolerance: real extractors always carry SE)
	const auto dir = writeFeatures(
		"UNITS=MM\n"
		"S P 2\n"
		"OB 0 0 I\n"
		"OS 1 1\n"
		"OE\n", "surface_eof");

	FeaturesFile features;
	ASSERT_TRUE(features.Parse(dir));
	ASSERT_EQ(features.GetFeatureRecords().size(), 1U);
	EXPECT_EQ(features.GetFeatureRecords().at(0)->type, FeaturesFile::FeatureRecord::Type::Surface);
}

TEST(FeaturesFileTests, Parse_SymbolDeclarations_UnitsAndDefaults)
{
	// "$" section (spec p.167): $<serial_num> <symbol_name> [<I|M>]
	// "The UNITS directive ... sets the units to be used for symbols that
	// do not specify units" -- both for symbols declared before and after
	// the UNITS line.
	const auto dir = writeFeatures(
		"$0 r120\n"
		"$1 rect20x60 M\n"
		"UNITS=MM\n"
		"$2 rect3x5 I\n"
		"$3 punch_target\n", "symbols_units");

	FeaturesFile features;
	ASSERT_TRUE(features.Parse(dir));

	const auto& symbols = features.GetSymbolNames();
	ASSERT_EQ(symbols.size(), 4U);

	// serial numbers are parsed in order from the $<n> token
	EXPECT_EQ(symbols.at(0)->GetIndex(), 0);
	EXPECT_EQ(symbols.at(1)->GetIndex(), 1);
	EXPECT_EQ(symbols.at(2)->GetIndex(), 2);
	EXPECT_EQ(symbols.at(3)->GetIndex(), 3);

	EXPECT_EQ(symbols.at(0)->GetName(), "r120");
	EXPECT_EQ(symbols.at(1)->GetName(), "rect20x60");
	EXPECT_EQ(symbols.at(2)->GetName(), "rect3x5");
	EXPECT_EQ(symbols.at(3)->GetName(), "punch_target");

	EXPECT_EQ(symbols.at(0)->GetUnitType(), Odb::Lib::UnitType::Metric);  // inherited from UNITS=MM
	EXPECT_EQ(symbols.at(1)->GetUnitType(), Odb::Lib::UnitType::Metric);  // explicit M
	EXPECT_EQ(symbols.at(2)->GetUnitType(), Odb::Lib::UnitType::Imperial); // explicit I overrides default
	EXPECT_EQ(symbols.at(3)->GetUnitType(), Odb::Lib::UnitType::Metric);  // inherited

	// both collections stay in sync
	EXPECT_EQ(features.GetSymbolNamesByName().size(), 4U);
	EXPECT_NE(features.GetSymbolNamesByName().find("rect3x5"), features.GetSymbolNamesByName().end());
	EXPECT_EQ(features.GetUnits(), "MM");
}

TEST(FeaturesFileTests, Parse_AltUnitsForm_SetsUnits)
{
	// the parser also accepts a second, space-separated units form
	// ("U MM"); pinned as an accepted input form (the spec form is
	// UNITS=MM)
	const auto dir = writeFeatures(
		"U INCH\n"
		"L 0 0 1 1 0 P 0\n", "alt_units_form");

	FeaturesFile features;
	ASSERT_TRUE(features.Parse(dir));
	EXPECT_EQ(features.GetUnits(), "INCH");
	EXPECT_EQ(features.GetSymbolNames().size(), 0U);
}

TEST(FeaturesFileTests, Parse_NumFeaturesAndId_HeaderFields)
{
	// header records (spec p.165): UNITS=<units>, ID=<id>, F <count>
	const auto dir = writeFeatures(
		"UNITS=MM\n"
		"ID=216\n"
		"F 3\n"
		"L 0 0 1 1 0 P 0\n"
		"P 0 0 0 P 0 0\n"
		"A 0 0 1 1 0.5 0.5 0 P 0 Y\n", "header_fields");

	FeaturesFile features;
	ASSERT_TRUE(features.Parse(dir));
	EXPECT_EQ(features.GetId(), 216U);
	EXPECT_EQ(features.GetNumFeatures(), 3);
	EXPECT_EQ(features.GetFeatureRecords().size(), 3U);
}

TEST(FeaturesFileTests, Parse_NoIdLine_DefaultIsSentinel)
{
	// default id is (unsigned)-1 when the ID record is absent
	const auto dir = writeFeatures("UNITS=MM\n", "no_id");

	FeaturesFile features;
	ASSERT_TRUE(features.Parse(dir));
	EXPECT_EQ(features.GetId(), std::numeric_limits<unsigned int>::max());
	EXPECT_EQ(features.GetNumFeatures(), 0);
}

TEST(FeaturesFileTests, Parse_AttributeLookupTables_Accepted)
{
	// "@"/"&" sections (spec p.168): attribute names and text strings.
	// The parsed tables are internal to the file object, so this test
	// pins acceptance and that they do not disturb feature parsing.
	const auto dir = writeFeatures(
		"UNITS=MM\n"
		"@0 .smd\n"
		"@1 .nomenclature\n"
		"&0 9796334\n"
		"&1 fid_0_0_0\n"
		"L 0 0 1 1 0 P 0 ;0,1=1\n", "attribute_tables");

	FeaturesFile features;
	ASSERT_TRUE(features.Parse(dir));
	ASSERT_EQ(features.GetFeatureRecords().size(), 1U);
	const auto attributes = snapshotAttributes(*features.GetFeatureRecords().at(0));
	EXPECT_EQ(attributes.at("0"), "");
	EXPECT_EQ(attributes.at("1"), "1");
}

TEST(FeaturesFileTests, Parse_MalformedIdValue_FailsLoudly)
{
	// "ID=abc" must not silently keep the sentinel id
	const auto dir = writeFeatures("ID=abc\n", "bad_id");

	FeaturesFile features;
	EXPECT_THROW(features.Parse(dir), std::exception);
}

TEST(FeaturesFileTests, Parse_MalformedNumFeatures_FailsLoudly)
{
	const auto dir = writeFeatures("F notanumber\n", "bad_num_features");

	FeaturesFile features;
	EXPECT_THROW(features.Parse(dir), std::exception);
}

TEST(FeaturesFileTests, Parse_TruncatedRecords_FailWithParseError)
{
	// each record below drops a required field mid-stream; the parser must
	// raise parse_error with context rather than emit a partial record
	const std::vector<std::pair<std::string, std::string>> truncatedRecords = {
		{ "line_no_xs", "L\n" },
		{ "line_no_end", "L 1.0 2.0\n" },
		{ "line_no_dcode", "L 1.0 2.0 3.0 4.0 0 P\n" },
		{ "line_bad_polarity", "L 1.0 2.0 3.0 4.0 0 X 5\n" },
		{ "pad_no_polarity", "P 1.0 2.0 0\n" },
		{ "pad_bad_polarity", "P 1.0 2.0 0 Q 4 0\n" },
		{ "arc_no_cw", "A 0 0 1 1 0.5 0.5 0 P 0\n" },
		{ "arc_bad_cw", "A 0 0 1 1 0.5 0.5 0 P 0 W\n" },
		{ "text_no_text", "T 1 2 standard P 0 1.0 1.0 1.0\n" },
		{ "text_unclosed_quote", "T 1 2 standard P 0 1.0 1.0 1.0 'oops 1\n" },
	};

	int caseIndex = 0;
	for (const auto& [tag, record] : truncatedRecords)
	{
		const auto dir = writeFeatures(
			"UNITS=MM\n" + record, "truncated_" + std::to_string(caseIndex++));
		FeaturesFile features;
		EXPECT_THROW(features.Parse(dir), Odb::Lib::FileModel::parse_error)
			<< "case " << tag << " was not rejected";
	}
}

TEST(FeaturesFileTests, Parse_MalformedSurfaceStructure_FailsWithParseError)
{
	const std::vector<std::pair<std::string, std::string>> malformedSurfaces = {
		// OE with no open surface record
		{ "oe_without_surface", "OE\n" },
		// SE with no open surface record
		{ "se_without_surface", "SE\n" },
		// OS with no open contour polygon
		{ "os_without_ob", "OS 1.0 1.0\n" },
		// OB with neither island nor hole token
		{ "ob_bad_type", "S P 0\nOB 0 0 X\nOE\nSE\n" },
		// polygon arc part with a bad clockwise flag
		{ "oc_bad_cw", "S P 0\nOB 0 0 I\nOC 1 1 0.5 0.5 q\nOE\nSE\n" },
	};

	int caseIndex = 0;
	for (const auto& [tag, contents] : malformedSurfaces)
	{
		const auto dir = writeFeatures(
			"UNITS=MM\n" + contents, "bad_surface_" + std::to_string(caseIndex++));
		FeaturesFile features;
		EXPECT_THROW(features.Parse(dir), Odb::Lib::FileModel::parse_error)
			<< "case " << tag << " was not rejected";
	}
}

TEST(FeaturesFileTests, Parse_BarcodeRecord_NoOpContract)
{
	// Barcode parsing is a stub (spec p.176 defines the full B grammar;
	// the parser reads only the leading token and ignores everything
	// after). Pinned contract: a well-formed barcode line yields exactly
	// one feature record of type Barcode with no fields populated -- not a
	// claim that the barcode parameters were parsed.
	const auto dir = writeFeatures(
		"UNITS=MM\n"
		"B 1.0 2.0 UPC39 standard P 8 E 0.1 0.2 N N N 0 0 ABC;1=2;ID=123\n", "barcode_full");

	FeaturesFile features;
	ASSERT_TRUE(features.Parse(dir));
	ASSERT_EQ(features.GetFeatureRecords().size(), 1U);

	const auto& barcode = *features.GetFeatureRecords().at(0);
	EXPECT_EQ(barcode.type, FeaturesFile::FeatureRecord::Type::Barcode);
	// the stub reads nothing else: coordinates, polarity, dcode and the
	// ;ID= trailer all stay unparsed defaults
	EXPECT_DOUBLE_EQ(barcode.x, 0.0);
	EXPECT_DOUBLE_EQ(barcode.y, 0.0);
	EXPECT_EQ(barcode.polarity, Odb::Lib::Polarity::Positive);
	EXPECT_EQ(barcode.dcode, 0);
	EXPECT_FALSE(barcode.id.has_value());
	EXPECT_TRUE(barcode.GetAttributeLookupTable().empty());
}

TEST(FeaturesFileTests, Parse_BarcodeAnyPayload_AcceptedSilently)
{
	// the stub validates nothing after the leading B token
	const auto dir = writeFeatures("B total-garbage\n", "barcode_garbage");

	FeaturesFile features;
	ASSERT_TRUE(features.Parse(dir));
	ASSERT_EQ(features.GetFeatureRecords().size(), 1U);
	EXPECT_EQ(features.GetFeatureRecords().at(0)->type, FeaturesFile::FeatureRecord::Type::Barcode);
}

TEST(FeaturesFileTests, Parse_UnrecognizedLine_WarnedAndSkipped)
{
	// documented tolerance: a line matching no record token is logged as
	// an unrecognized line and skipped -- it is not a parse error
	const auto dir = writeFeatures(
		"UNITS=MM\n"
		"ZEBRA 123 nonsense\n"
		"L 0 0 1 1 0 P 0\n", "unrecognized_line");

	FeaturesFile features;
	ASSERT_TRUE(features.Parse(dir));
	ASSERT_EQ(features.GetFeatureRecords().size(), 1U);
	EXPECT_EQ(features.GetFeatureRecords().at(0)->type, FeaturesFile::FeatureRecord::Type::Line);
}

TEST(FeaturesFileTests, Parse_CommentsAndBlankLines_Skipped)
{
	const auto dir = writeFeatures(
		"# header comment\n"
		"\n"
		"UNITS=MM\n"
		"# another comment with L 1 2 3 4 5 P 0 inside\n"
		"L 0 0 1 1 0 P 0\n"
		"\n", "comments");

	FeaturesFile features;
	ASSERT_TRUE(features.Parse(dir));
	ASSERT_EQ(features.GetFeatureRecords().size(), 1U);
	EXPECT_DOUBLE_EQ(features.GetFeatureRecords().at(0)->xs, 0.0);
}

TEST(FeaturesFileTests, Parse_AlternateBuiltinFilename_Features2Found)
{
	// the parser probes features, features2, features3 in order
	const auto dir = writeFeatures(
		"UNITS=MM\n"
		"L 0 0 1 1 0 P 0\n", "features2_probe", "features2");

	FeaturesFile features;
	ASSERT_TRUE(features.Parse(dir));
	EXPECT_EQ(features.GetPath().filename().string(), "features2");
	ASSERT_EQ(features.GetFeatureRecords().size(), 1U);
}

TEST(FeaturesFileTests, Parse_AlternateFilenameParameter_UsedExclusively)
{
	// with an explicit alternateFilename only that name is probed
	const auto dir = writeFeatures(
		"UNITS=MM\n"
		"L 0 0 1 1 0 P 0\n", "explicit_alt", "mylayer_features");

	FeaturesFile features;
	ASSERT_TRUE(features.Parse(dir, "mylayer_features"));
	EXPECT_EQ(features.GetPath().filename().string(), "mylayer_features");
	ASSERT_EQ(features.GetFeatureRecords().size(), 1U);
}

TEST(FeaturesFileTests, Parse_NoFeaturesFile_ThrowsInvalidOdbError)
{
	const auto dir = fs::temp_directory_path() / "odbdesign_featuresfile_tests" / "empty_dir";
	fs::create_directories(dir);

	// the parser throws invalid_odb_error ("features file does not exist"),
	// but its generic `catch (std::exception&) { ...; throw e; }` handler
	// re-throws by value and SLICES it to a bare std::exception -- the
	// concrete type and message are lost. Pinned as the observed contract.
	FeaturesFile features;
	EXPECT_THROW(features.Parse(dir), std::exception);
}

TEST(FeaturesFileTests, Parse_MalformedIdTrailer_SingleSemicolon_MisroutedToAttributeMap)
{
	// The trailer string arrives WITH its leading ';' intact, so for the
	// single-';' form ";ID=abc" ParseAttributeLookupTable treats
	// "ID=abc" as the ATTRIBUTES section (the same single-';' misroute
	// pinned on surfaces above): the record id stays unset and a fake
	// "ID"="abc" attribute is added. Rerouting the uid must flip both
	// assertions.
	const auto dir = writeFeatures(
		"UNITS=MM\n"
		"L 0 0 1 1 0 P 0 ;ID=abc\n", "bad_id_trailer");

	FeaturesFile features;
	ASSERT_TRUE(features.Parse(dir));
	ASSERT_EQ(features.GetFeatureRecords().size(), 1U);

	const auto& line = *features.GetFeatureRecords().at(0);
	EXPECT_FALSE(line.id.has_value());
	const auto attributes = snapshotAttributes(line);
	ASSERT_EQ(attributes.size(), 1U);
	EXPECT_EQ(attributes.at("ID"), "abc");
}

TEST(FeaturesFileTests, Parse_MalformedIdTrailer_DoubleSemicolon_FromCharsRejectsSilently)
{
	// with the explicit empty attributes section (";;ID=abc") the value
	// DOES reach the from_chars-guarded uid section, which silently skips
	// invalid values: id stays unset, no attributes
	const auto dir = writeFeatures(
		"UNITS=MM\n"
		"L 0 0 1 1 0 P 0 ;;ID=abc\n", "bad_id_trailer_double");

	FeaturesFile features;
	ASSERT_TRUE(features.Parse(dir));
	ASSERT_EQ(features.GetFeatureRecords().size(), 1U);

	const auto& line = *features.GetFeatureRecords().at(0);
	EXPECT_FALSE(line.id.has_value());
	EXPECT_TRUE(line.GetAttributeLookupTable().empty());
}

TEST(FeaturesFileTests, ProtobufRoundTrip_PreservesAllRecordsAndSymbols)
{
	const auto dir = writeFeatures(
		"UNITS=MM\n"
		"ID=216\n"
		"F 4\n"
		"$0 r120 M\n"
		"$1 punch_target\n"
		"L 3.834 -1.16 3.86728 -1.16485 2 N 5 ;1,3=0;ID=4321\n"
		"P 1.0 2.0 -1 0.02 P 4 1;;ID=7421\n"
		"T 11.890963 13.697185 standard P 8 30.0 5.08 5.08 2.0 '$$DATE-MMDDYY' 1;2=1;ID=777\n"
		"A 1.49401033 2.53683297 0.8389936 2.48563819 1.17858858 2.35659203 2 P 0 Y\n"
		"S P 0;;ID=123456\n"
		"OB -0.013 2.427 I\n"
		"OS -0.013 2.218\n"
		"OE\n"
		"OB 0.5 0.5 H\n"
		"OC 1.0 1.0 0.75 0.75 n\n"
		"OE\n"
		"SE\n", "protobuf_roundtrip");

	FeaturesFile original;
	ASSERT_TRUE(original.Parse(dir));
	auto originalMessage = original.to_protobuf();
	ASSERT_NE(originalMessage, nullptr);

	FeaturesFile roundTripped;
	roundTripped.from_protobuf(*originalMessage);
	auto roundTrippedMessage = roundTripped.to_protobuf();

	EXPECT_EQ(roundTrippedMessage->units(), "MM");
	EXPECT_EQ(roundTrippedMessage->id(), 216U);
	EXPECT_EQ(roundTrippedMessage->numfeatures(), 4);
	ASSERT_EQ(roundTrippedMessage->featurerecords_size(), originalMessage->featurerecords_size());
	ASSERT_EQ(roundTrippedMessage->featurerecords_size(), 5);

	for (int i = 0; i < roundTrippedMessage->featurerecords_size(); ++i)
	{
		const auto& lhs = originalMessage->featurerecords(i);
		const auto& rhs = roundTrippedMessage->featurerecords(i);
		EXPECT_EQ(lhs.type(), rhs.type()) << "record " << i;
		EXPECT_DOUBLE_EQ(lhs.xs(), rhs.xs()) << "record " << i;
		EXPECT_DOUBLE_EQ(lhs.ys(), rhs.ys()) << "record " << i;
		EXPECT_DOUBLE_EQ(lhs.xe(), rhs.xe()) << "record " << i;
		EXPECT_DOUBLE_EQ(lhs.ye(), rhs.ye()) << "record " << i;
		EXPECT_DOUBLE_EQ(lhs.x(), rhs.x()) << "record " << i;
		EXPECT_DOUBLE_EQ(lhs.y(), rhs.y()) << "record " << i;
		EXPECT_DOUBLE_EQ(lhs.xc(), rhs.xc()) << "record " << i;
		EXPECT_DOUBLE_EQ(lhs.yc(), rhs.yc()) << "record " << i;
		EXPECT_EQ(lhs.cw(), rhs.cw()) << "record " << i;
		EXPECT_EQ(lhs.font(), rhs.font()) << "record " << i;
		EXPECT_EQ(lhs.text(), rhs.text()) << "record " << i;
		EXPECT_EQ(lhs.version(), rhs.version()) << "record " << i;
		EXPECT_EQ(lhs.sym_num(), rhs.sym_num()) << "record " << i;
		EXPECT_EQ(lhs.polarity(), rhs.polarity()) << "record " << i;
		EXPECT_EQ(lhs.dcode(), rhs.dcode()) << "record " << i;
		EXPECT_EQ(lhs.id(), rhs.id()) << "record " << i;
		EXPECT_EQ(lhs.orient_def(), rhs.orient_def()) << "record " << i;
		EXPECT_DOUBLE_EQ(lhs.orient_def_rotation(), rhs.orient_def_rotation()) << "record " << i;
		EXPECT_EQ(lhs.contourpolygons_size(), rhs.contourpolygons_size()) << "record " << i;
	}

	// deep spot-checks on the interesting records
	const auto& padMessage = roundTrippedMessage->featurerecords(1);
	// to_protobuf omits apt_def_symbol_num when it is -1 (optional field):
	// absence on the wire is the sentinel's encoding
	EXPECT_FALSE(padMessage.has_apt_def_symbol_num());
	EXPECT_DOUBLE_EQ(padMessage.apt_def_resize_factor(), 0.02);

	const auto& textMessage = roundTrippedMessage->featurerecords(2);
	EXPECT_EQ(textMessage.type(), ProtobufFeaturesFile::FeatureRecord::Text);
	EXPECT_DOUBLE_EQ(textMessage.orient_def_rotation(), 30.0);
	EXPECT_EQ(textMessage.text(), "$$DATE-MMDDYY");

	const auto& surfaceMessage = roundTrippedMessage->featurerecords(4);
	ASSERT_EQ(surfaceMessage.contourpolygons_size(), 2);
	EXPECT_EQ(surfaceMessage.contourpolygons(0).type(), Odb::Lib::Protobuf::ContourPolygon::Island);
	EXPECT_EQ(surfaceMessage.contourpolygons(1).type(), Odb::Lib::Protobuf::ContourPolygon::Hole);
	EXPECT_EQ(surfaceMessage.contourpolygons(1).polygonparts(0).type(),
		Odb::Lib::Protobuf::ContourPolygon::PolygonPart::Arc);
	EXPECT_TRUE(surfaceMessage.contourpolygons(1).polygonparts(0).isclockwise() == false);

	// symbols survive the round trip (iterated, never find() on the
	// protobuf map)
	const auto originalSymbols = snapshotSymbols(*originalMessage);
	const auto roundTrippedSymbols = snapshotSymbols(*roundTrippedMessage);
	ASSERT_EQ(roundTrippedSymbols.size(), originalSymbols.size());
	for (const auto& kv : originalSymbols)
	{
		ASSERT_NE(roundTrippedSymbols.find(kv.first), roundTrippedSymbols.end());
		EXPECT_EQ(roundTrippedSymbols.at(kv.first)->name(), kv.second->name());
		EXPECT_EQ(roundTrippedSymbols.at(kv.first)->unittype(), kv.second->unittype());
	}
}

TEST(FeaturesFileTests, ProtobufRoundTrip_AbsentOptionalId_StaysAbsent)
{
	// a record with no ;ID= trailer must not acquire id=0 over the wire
	const auto dir = writeFeatures(
		"UNITS=MM\n"
		"L 0 0 1 1 0 P 0\n", "optional_id_absence");

	FeaturesFile original;
	ASSERT_TRUE(original.Parse(dir));
	auto message = original.to_protobuf();
	ASSERT_EQ(message->featurerecords_size(), 1);
	EXPECT_FALSE(message->featurerecords(0).has_id());

	FeaturesFile roundTripped;
	roundTripped.from_protobuf(*message);
	ASSERT_EQ(roundTripped.GetFeatureRecords().size(), 1U);
	EXPECT_FALSE(roundTripped.GetFeatureRecords().at(0)->id.has_value());
}
