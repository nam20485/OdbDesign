#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "str_utils.h"

using namespace Utils;

namespace Odb::Test
{
    // ---- in-place whitespace trims -------------------------------------

    TEST(StringUtilsTests, Trim_EmptyString_StaysEmpty)
    {
        std::string s;
        EXPECT_EQ(str_trim(s), "");
        EXPECT_EQ(str_ltrim(s), "");
        EXPECT_EQ(str_rtrim(s), "");
    }

    TEST(StringUtilsTests, Trim_InPlace_KnownVectors)
    {
        std::string l = "  hello  ";
        EXPECT_EQ(str_ltrim(l), "hello  ");

        std::string r = "  hello  ";
        EXPECT_EQ(str_rtrim(r), "  hello");

        std::string b = "  hello  ";
        EXPECT_EQ(str_trim(b), "hello");
    }

    TEST(StringUtilsTests, Trim_AllWhitespaceCharacters_SetCovered)
    {
        // std::isspace set: space, \t, \n, \v, \f, \r
        std::string s = " \t\n\v\f\r";
        EXPECT_EQ(str_trim(s), "");

        std::string mixed = "\t\ntrimmed\r\v\f";
        EXPECT_EQ(str_trim(mixed), "trimmed");
    }

    TEST(StringUtilsTests, Trim_HighBitByte_NotWhitespace)
    {
        // 0xB5 is not a whitespace char under the C locale and must survive
        std::string s = "\xB5keep\xB5";
        EXPECT_EQ(str_trim(s), "\xB5keep\xB5");
    }

    // ---- copying trims: source must stay untouched ----------------------

    TEST(StringUtilsTests, TrimCopy_KnownVectors_SourceUnchanged)
    {
        const std::string original = "  padded  ";
        EXPECT_EQ(str_ltrim_copy(original), "padded  ");
        EXPECT_EQ(str_rtrim_copy(original), "  padded");
        EXPECT_EQ(str_trim_copy(original), "padded");
        // the copy variants must leave the source string alone
        EXPECT_EQ(original, "  padded  ");
    }

    TEST(StringUtilsTests, TrimCopy_EmptyString_StaysEmpty)
    {
        const std::string empty;
        EXPECT_EQ(str_trim_copy(empty), "");
        EXPECT_EQ(str_ltrim_copy(empty), "");
        EXPECT_EQ(str_rtrim_copy(empty), "");
    }

    // ---- character-specific trims ---------------------------------------

    TEST(StringUtilsTests, TrimChar_KnownVectors)
    {
        EXPECT_EQ(str_trim_copy("---value---", '-'), "value");
        EXPECT_EQ(str_ltrim_copy("---value---", '-'), "value---");
        EXPECT_EQ(str_rtrim_copy("---value---", '-'), "---value");
        // only the exact character is trimmed
        EXPECT_EQ(str_trim_copy("xx-a-xx", 'x'), "-a-");
        // nothing to trim -> unchanged
        EXPECT_EQ(str_trim_copy("value", '-'), "value");
    }

    // ---- case conversion (in place) --------------------------------------

    TEST(StringUtilsTests, ToLower_InPlace_KnownVector)
    {
        std::string s = "MiXeD123!@#";
        str_to_lower(s);
        EXPECT_EQ(s, "mixed123!@#");
    }

    TEST(StringUtilsTests, ToUpper_InPlace_KnownVector)
    {
        std::string s = "MiXeD123!@#";
        str_to_upper(s);
        EXPECT_EQ(s, "MIXED123!@#");
    }

    TEST(StringUtilsTests, ToLowerToUpper_InPlace_EmptyString)
    {
        std::string s;
        str_to_lower(s);
        EXPECT_EQ(s, "");
        str_to_upper(s);
        EXPECT_EQ(s, "");
    }

    // ---- case conversion (copies) ----------------------------------------

    TEST(StringUtilsTests, ToLowerCopy_KnownVector_SourceUnchanged)
    {
        const std::string original = "MiXeD";
        EXPECT_EQ(str_to_lower_copy(original), "mixed");
        EXPECT_EQ(original, "MiXeD");
    }

    // REGRESSION GUARD (was a known defect, fixed in this PR):
    // str_to_upper_copy() used to transform an EMPTY string and return ""
    // for every input; it now copies the source first (mirroring
    // str_to_lower_copy).
    TEST(StringUtilsTests, ToUpperCopy_KnownVector_SourceUnchanged)
    {
        const std::string original = "MiXeD123";
        EXPECT_EQ(str_to_upper_copy(original), "MIXED123");
        EXPECT_EQ(original, "MiXeD123");
    }

    TEST(StringUtilsTests, ToUpperCopy_EmptyString_ReturnsEmpty)
    {
        EXPECT_EQ(str_to_upper_copy(""), "");
    }

    // ---- case-insensitive comparison --------------------------------------

    TEST(StringUtilsTests, IEquals_DifferingCase_True)
    {
        EXPECT_TRUE(str_iequals("hello", "HELLO"));
        EXPECT_TRUE(str_iequals("HeLLo", "hello"));
        EXPECT_TRUE(str_iequals("", ""));
        EXPECT_TRUE(str_iequals("abc123!@#", "ABC123!@#"));
    }

    TEST(StringUtilsTests, IEquals_DifferentContent_False)
    {
        EXPECT_FALSE(str_iequals("hello", "hell"));
        EXPECT_FALSE(str_iequals("hell", "hello"));
        EXPECT_FALSE(str_iequals("", "a"));
        EXPECT_FALSE(str_iequals("a", ""));
        EXPECT_FALSE(str_iequals("apple", "APPLE ")); // trailing space matters
    }

    TEST(StringUtilsTests, IEquals_HighBitBytes_CaseSensitiveInCLocale)
    {
        // Documented actual behavior: tolower() only folds ASCII under the C
        // locale, so Latin-1 letters do NOT compare case-insensitively.
        // (0xC9 = 'É', 0xE9 = 'é' as single CP1252 bytes.)
        EXPECT_FALSE(str_iequals(std::string(1, '\xC9'), std::string(1, '\xE9')));
        EXPECT_TRUE(str_iequals(std::string(1, '\xE9'), std::string(1, '\xE9')));
    }

    // ---- find_str_icmp -----------------------------------------------------

    TEST(StringUtilsTests, FindStrIcmp_FoundCaseInsensitive)
    {
        std::vector<std::string> values{ "Alpha", "beta", "GAMMA" };
        auto it = find_str_icmp(values.begin(), values.end(), "BETA");
        ASSERT_NE(it, values.end());
        EXPECT_EQ(*it, "beta");
        EXPECT_EQ(std::distance(values.begin(), it), 1);
    }

    TEST(StringUtilsTests, FindStrIcmp_NotFound_ReturnsEnd)
    {
        std::vector<std::string> values{ "Alpha", "beta" };
        EXPECT_EQ(find_str_icmp(values.begin(), values.end(), "delta"), values.end());
        EXPECT_EQ(find_str_icmp(values.begin(), values.end(), ""), values.end());
    }

    TEST(StringUtilsTests, FindStrIcmp_EmptyVector_ReturnsEnd)
    {
        std::vector<std::string> values;
        EXPECT_EQ(find_str_icmp(values.begin(), values.end(), "anything"), values.end());
    }

    TEST(StringUtilsTests, FindStrIcmp_ConstOverload_Found)
    {
        const std::vector<std::string> values{ "one", "TWO", "three" };
        auto it = find_str_icmp(values.cbegin(), values.cend(), "two");
        ASSERT_NE(it, values.cend());
        EXPECT_EQ(*it, "TWO");
    }
} // namespace Odb::Test
