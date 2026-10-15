#include <gtest/gtest.h>
#include <FileModel/Design/AttributeLookupTable.h>

using namespace Odb::Lib::FileModel::Design;

namespace
{
	// AttributeLookupTable is abstract (protected ctor); derive to unit-test the parse directly
	class TestAttributeLookupTable : public AttributeLookupTable
	{
	public:
		TestAttributeLookupTable() = default;
	};
}

// The attribute string is optional on ODB++ records: when a record carries none,
// callers pass an empty string and must not treat that as malformed data.
TEST(AttributeLookupTableTests, ParseAttributeLookupTable_EmptyString_ReturnsTrueWithEmptyTable)
{
	TestAttributeLookupTable alt;
	EXPECT_TRUE(alt.ParseAttributeLookupTable(""));
	EXPECT_TRUE(alt.GetAttributeLookupTable().empty());
}

TEST(AttributeLookupTableTests, ParseAttributeLookupTable_SingleSemicolon_ReturnsTrueWithEmptyTable)
{
	TestAttributeLookupTable alt;
	EXPECT_TRUE(alt.ParseAttributeLookupTable(";"));
	EXPECT_TRUE(alt.GetAttributeLookupTable().empty());
}

TEST(AttributeLookupTableTests, ParseAttributeLookupTable_TwoSemicolons_ReturnsTrueWithEmptyTable)
{
	TestAttributeLookupTable alt;
	EXPECT_TRUE(alt.ParseAttributeLookupTable(";;"));
	EXPECT_TRUE(alt.GetAttributeLookupTable().empty());
}

// A string with no semicolon is a single (skipped) first section: the stream ran
// out after it, which is "no attributes", not malformed data.
TEST(AttributeLookupTableTests, ParseAttributeLookupTable_NoSemicolon_ReturnsTrueWithEmptyTable)
{
	TestAttributeLookupTable alt;
	EXPECT_TRUE(alt.ParseAttributeLookupTable("abc"));
	EXPECT_TRUE(alt.GetAttributeLookupTable().empty());
}

// Section semantics: <skipped>;<name=value,...>
TEST(AttributeLookupTableTests, ParseAttributeLookupTable_AttributesOnly_ParsesAttributePairs)
{
	TestAttributeLookupTable alt;
	EXPECT_TRUE(alt.ParseAttributeLookupTable("0;1=4,3=1"));

	const auto& table = alt.GetAttributeLookupTable();
	EXPECT_EQ(table.size(), static_cast<size_t>(2));
	EXPECT_EQ(table.at("1"), "4");
	EXPECT_EQ(table.at("3"), "1");
}

// Section semantics: <skipped>;<name=value,...>;<ID name=value>
TEST(AttributeLookupTableTests, ParseAttributeLookupTable_AttributesAndId_ParsesAllSections)
{
	TestAttributeLookupTable alt;
	EXPECT_TRUE(alt.ParseAttributeLookupTable("0;1=4,3=1;ID=5"));

	const auto& table = alt.GetAttributeLookupTable();
	EXPECT_EQ(table.size(), static_cast<size_t>(3));
	EXPECT_EQ(table.at("1"), "4");
	EXPECT_EQ(table.at("3"), "1");
	EXPECT_EQ(table.at("ID"), "5");
}
