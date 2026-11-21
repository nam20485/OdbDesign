#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <vector>

#include "EnumMap.h"

using ::Utils::EnumMap;

namespace Odb::Test
{
    namespace
    {
        enum class TestColor
        {
            Red,
            Green,
            Blue,
        };

        const std::vector<std::string> colorNames{ "Red", "Green", "Blue" };
    }

    // ---- enum -> name -------------------------------------------------------

    TEST(EnumMapTests, GetValueByEnum_ReturnsName)
    {
        const EnumMap<TestColor> map(colorNames);
        EXPECT_EQ(map.getValue(TestColor::Red), "Red");
        EXPECT_EQ(map.getValue(TestColor::Green), "Green");
        EXPECT_EQ(map.getValue(TestColor::Blue), "Blue");
    }

    TEST(EnumMapTests, GetValueByEnum_OutOfRange_ThrowsInvalidArgument)
    {
        const EnumMap<TestColor> map(colorNames);
        EXPECT_THROW(map.getValue(static_cast<TestColor>(99)), std::invalid_argument);
    }

    TEST(EnumMapTests, GetValueByEnum_NegativeValue_ThrowsInvalidArgument)
    {
        const EnumMap<TestColor> map(colorNames);
        // the negative index is not representable in size_t; the guard must
        // still throw rather than read out of bounds
        EXPECT_THROW(map.getValue(static_cast<TestColor>(-1)), std::invalid_argument);
    }

    // ---- name -> enum -------------------------------------------------------

    TEST(EnumMapTests, GetValueByName_ReturnsEnum)
    {
        const EnumMap<TestColor> map(colorNames);
        EXPECT_EQ(map.getValue("Red"), TestColor::Red);
        EXPECT_EQ(map.getValue("Green"), TestColor::Green);
        EXPECT_EQ(map.getValue("Blue"), TestColor::Blue);
    }

    TEST(EnumMapTests, GetValueByName_UnknownName_ThrowsInvalidArgument)
    {
        const EnumMap<TestColor> map(colorNames);
        try
        {
            (void)map.getValue("Purple");
            FAIL() << "expected std::invalid_argument";
        }
        catch (const std::invalid_argument& e)
        {
            EXPECT_NE(std::string(e.what()).find("Purple"), std::string::npos);
        }
    }

    TEST(EnumMapTests, GetValueByName_CaseSensitiveByDefault)
    {
        const EnumMap<TestColor> map(colorNames);
        EXPECT_THROW(map.getValue("red"), std::invalid_argument);
        EXPECT_THROW(map.getValue("RED"), std::invalid_argument);
        EXPECT_FALSE(map.contains("red"));
    }

    // ---- contains -------------------------------------------------------------

    TEST(EnumMapTests, ContainsByName_TrueForKnown_FalseForUnknown)
    {
        const EnumMap<TestColor> map(colorNames);
        EXPECT_TRUE(map.contains("Red"));
        EXPECT_TRUE(map.contains("Blue"));
        EXPECT_FALSE(map.contains("red"));
        EXPECT_FALSE(map.contains(""));
        EXPECT_FALSE(map.contains("Purple"));
    }

    TEST(EnumMapTests, ContainsByEnum_TrueInBounds_FalseOutOfRange)
    {
        const EnumMap<TestColor> map(colorNames);
        EXPECT_TRUE(map.contains(TestColor::Red));
        EXPECT_TRUE(map.contains(TestColor::Blue));
        EXPECT_FALSE(map.contains(static_cast<TestColor>(3)));
        EXPECT_FALSE(map.contains(static_cast<TestColor>(99)));
        EXPECT_FALSE(map.contains(static_cast<TestColor>(-1)));
    }

    // ---- case-insensitive mode -------------------------------------------------

    TEST(EnumMapTests, CaseInsensitive_GetValueByName_MatchesAnyCase)
    {
        const EnumMap<TestColor> map(colorNames, /*caseInsensitive=*/true);
        EXPECT_EQ(map.getValue("red"), TestColor::Red);
        EXPECT_EQ(map.getValue("RED"), TestColor::Red);
        EXPECT_EQ(map.getValue("gReEn"), TestColor::Green);
    }

    // KNOWN DEFECT TRIPWIRE (currently FAILS): the string overload of
    // contains() ignores the caseInsensitive flag and always does an exact
    // find (Utils/EnumMap.h:72-75), unlike getValue(string) which honors it.
    // This test asserts the CORRECT behavior; fixing the implementation
    // should make it pass.
    TEST(EnumMapTests, CaseInsensitive_Contains_HonorsCaseInsensitiveFlag)
    {
        const EnumMap<TestColor> map(colorNames, /*caseInsensitive=*/true);
        EXPECT_TRUE(map.contains("red"));
        EXPECT_TRUE(map.contains("RED"));
        EXPECT_TRUE(map.contains("Red"));
        EXPECT_FALSE(map.contains("purple"));
    }
} // namespace Odb::Test
