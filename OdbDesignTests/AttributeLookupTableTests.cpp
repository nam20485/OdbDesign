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
TEST(AttributeLookupTableTests, ParseAttributeLookupTable_EmptyString_ReturnsTrueWithEmptyTableAndNoId)
{
	TestAttributeLookupTable alt;
	std::optional<unsigned int> id;
	EXPECT_TRUE(alt.ParseAttributeLookupTable("", id));
	EXPECT_TRUE(alt.GetAttributeLookupTable().empty());
	EXPECT_FALSE(id.has_value());
}

TEST(AttributeLookupTableTests, ParseAttributeLookupTable_SingleSemicolon_ReturnsTrueWithEmptyTableAndNoId)
{
	TestAttributeLookupTable alt;
	std::optional<unsigned int> id;
	EXPECT_TRUE(alt.ParseAttributeLookupTable(";", id));
	EXPECT_TRUE(alt.GetAttributeLookupTable().empty());
	EXPECT_FALSE(id.has_value());
}

TEST(AttributeLookupTableTests, ParseAttributeLookupTable_TwoSemicolons_ReturnsTrueWithEmptyTableAndNoId)
{
	TestAttributeLookupTable alt;
	std::optional<unsigned int> id;
	EXPECT_TRUE(alt.ParseAttributeLookupTable(";;", id));
	EXPECT_TRUE(alt.GetAttributeLookupTable().empty());
	EXPECT_FALSE(id.has_value());
}

// A string with no semicolon is a single (skipped) first section: the stream ran
// out after it, which is "no attributes", not malformed data.
TEST(AttributeLookupTableTests, ParseAttributeLookupTable_NoSemicolon_ReturnsTrueWithEmptyTableAndNoId)
{
	TestAttributeLookupTable alt;
	std::optional<unsigned int> id;
	EXPECT_TRUE(alt.ParseAttributeLookupTable("abc", id));
	EXPECT_TRUE(alt.GetAttributeLookupTable().empty());
	EXPECT_FALSE(id.has_value());
}

// Section semantics: <skipped>;<name=value,...>
TEST(AttributeLookupTableTests, ParseAttributeLookupTable_AttributesOnly_ParsesAttributePairsAndNoId)
{
	TestAttributeLookupTable alt;
	std::optional<unsigned int> id;
	EXPECT_TRUE(alt.ParseAttributeLookupTable("0;1=4,3=1", id));

	const auto& table = alt.GetAttributeLookupTable();
	EXPECT_EQ(table.size(), static_cast<size_t>(2));
	EXPECT_EQ(table.at("1"), "4");
	EXPECT_EQ(table.at("3"), "1");
	EXPECT_FALSE(id.has_value());
}

// Section semantics: <skipped>;<name=value,...>;<ID=uid>. The id is NOT an
// attribute: it must come back only through the typed out-parameter, never as a
// literal "ID" key in the table (decision D3 -- one home for the UID).
TEST(AttributeLookupTableTests, ParseAttributeLookupTable_AttributesAndId_ParsesIdIntoOutParamOnly)
{
	TestAttributeLookupTable alt;
	std::optional<unsigned int> id;
	EXPECT_TRUE(alt.ParseAttributeLookupTable("0;1=4,3=1;ID=5", id));

	const auto& table = alt.GetAttributeLookupTable();
	EXPECT_EQ(table.size(), static_cast<size_t>(2));
	EXPECT_EQ(table.at("1"), "4");
	EXPECT_EQ(table.at("3"), "1");
	EXPECT_EQ(table.find("ID"), table.end());
	ASSERT_TRUE(id.has_value());
	EXPECT_EQ(*id, 5u);
}

// The third section alone (no attribute assignments) still yields the id.
TEST(AttributeLookupTableTests, ParseAttributeLookupTable_IdOnly_ReturnsIdWithEmptyTable)
{
	TestAttributeLookupTable alt;
	std::optional<unsigned int> id;
	EXPECT_TRUE(alt.ParseAttributeLookupTable("0;;ID=42", id));
	EXPECT_TRUE(alt.GetAttributeLookupTable().empty());
	ASSERT_TRUE(id.has_value());
	EXPECT_EQ(*id, 42u);
}

// A non-numeric id value must not fail the parse of otherwise-good data: the
// record still carries its attributes, and the id stays unset.
TEST(AttributeLookupTableTests, ParseAttributeLookupTable_NonNumericId_ReturnsTrueWithNoId)
{
	TestAttributeLookupTable alt;
	std::optional<unsigned int> id;
	EXPECT_TRUE(alt.ParseAttributeLookupTable("0;1=4;ID=abc", id));

	const auto& table = alt.GetAttributeLookupTable();
	EXPECT_EQ(table.size(), static_cast<size_t>(1));
	EXPECT_EQ(table.at("1"), "4");
	EXPECT_EQ(table.find("ID"), table.end());
	EXPECT_FALSE(id.has_value());
}

// Ids outside the uint32 range stay unset rather than wrapping.
TEST(AttributeLookupTableTests, ParseAttributeLookupTable_OutOfRangeId_ReturnsTrueWithNoId)
{
	TestAttributeLookupTable alt;
	std::optional<unsigned int> id;
	EXPECT_TRUE(alt.ParseAttributeLookupTable("0;;ID=99999999999", id));
	EXPECT_TRUE(alt.GetAttributeLookupTable().empty());
	EXPECT_FALSE(id.has_value());
}

// An out-of-spec name in the id section is preserved as attribute data rather
// than silently dropped; it never becomes an id.
TEST(AttributeLookupTableTests, ParseAttributeLookupTable_OutOfSpecThirdSectionName_PreservedAsAttribute)
{
	TestAttributeLookupTable alt;
	std::optional<unsigned int> id;
	EXPECT_TRUE(alt.ParseAttributeLookupTable("0;1=4;FOO=7", id));

	const auto& table = alt.GetAttributeLookupTable();
	EXPECT_EQ(table.size(), static_cast<size_t>(2));
	EXPECT_EQ(table.at("1"), "4");
	EXPECT_EQ(table.at("FOO"), "7");
	EXPECT_FALSE(id.has_value());
}

// The out-parameter is reset on every call: a reused optional cannot leak a
// previous record's id into a record that carries none.
TEST(AttributeLookupTableTests, ParseAttributeLookupTable_ReusedOutParam_DoesNotLeakPreviousId)
{
	TestAttributeLookupTable alt;
	std::optional<unsigned int> id;
	ASSERT_TRUE(alt.ParseAttributeLookupTable("0;;ID=5", id));
	ASSERT_TRUE(id.has_value());

	EXPECT_TRUE(alt.ParseAttributeLookupTable("0;1=4", id));
	EXPECT_FALSE(id.has_value());
}
