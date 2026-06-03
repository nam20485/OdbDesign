#include <gtest/gtest.h>
#include <string>
#include <string_view>
#include "../OdbDesignLib/Text/Utf8Sanitizer.h"
#include "OdbDesignLib/FileModel/Design/PropertyRecord.h"

using namespace Odb::Lib::Text;

namespace Odb::Test
{
    class Utf8SanitizerTest : public testing::Test
    {
    protected:
        // Helper to check byte-for-byte equality
        static bool BytesMatch(std::string_view s1, std::string_view s2)
        {
            if (s1.size() != s2.size()) return false;
            for (size_t i = 0; i < s1.size(); ++i)
            {
                if (s1[i] != s2[i]) return false;
            }
            return true;
        }
    };

    // 1. ASCII passes through unchanged
    TEST_F(Utf8SanitizerTest, AsciiPassthrough)
    {
        std::string input = "Hello, World! 123";
        EXPECT_TRUE(IsValidUtf8(input));
        EXPECT_EQ(ToUtf8(input), input);
    }

    // 2. Valid multi-byte UTF-8 passes through unchanged
    TEST_F(Utf8SanitizerTest, ValidUtf8Passthrough)
    {
        // Ö as 0xC3 0x96
        std::string utf8_oe = "\xC3\x96";
        EXPECT_TRUE(IsValidUtf8(utf8_oe));
        EXPECT_EQ(ToUtf8(utf8_oe), utf8_oe);

        // € as 0xE2 0x82 0xAC
        std::string utf8_euro = "\xE2\x82\xAC";
        EXPECT_TRUE(IsValidUtf8(utf8_euro));
        EXPECT_EQ(ToUtf8(utf8_euro), utf8_euro);

        // Emoji (4-byte UTF-8)
        std::string utf8_emoji = "\xF0\x9F\x98\x80";
        EXPECT_TRUE(IsValidUtf8(utf8_emoji));
        EXPECT_EQ(ToUtf8(utf8_emoji), utf8_emoji);
    }

    // 3. Single CP1252 byte 0xD6 -> UTF-8 0xC3 0x96 (Ö)
    TEST_F(Utf8SanitizerTest, Cp1252SingleByte_O)
    {
        std::string input = "\xD6";
        EXPECT_FALSE(IsValidUtf8(input));
        std::string expected = "\xC3\x96";
        EXPECT_TRUE(BytesMatch(ToUtf8(input), expected));
        EXPECT_TRUE(IsValidUtf8(ToUtf8(input)));
    }

    // 4. Single CP1252 byte 0x80 -> UTF-8 for € (U+20AC = 0xE2 0x82 0xAC)
    TEST_F(Utf8SanitizerTest, Cp1252SingleByte_Euro)
    {
        std::string input = "\x80";
        EXPECT_FALSE(IsValidUtf8(input));
        std::string expected = "\xE2\x82\xAC";
        EXPECT_TRUE(BytesMatch(ToUtf8(input), expected));
        EXPECT_TRUE(IsValidUtf8(ToUtf8(input)));
    }

    // 5. Undefined CP1252 byte 0x81 -> U+FFFD (0xEF 0xBF 0xBD)
    TEST_F(Utf8SanitizerTest, Cp1252UndefinedByte_Replacement)
    {
        std::string input = "\x81";
        EXPECT_FALSE(IsValidUtf8(input));
        std::string expected = "\xEF\xBF\xBD";
        EXPECT_TRUE(BytesMatch(ToUtf8(input), expected));
        EXPECT_TRUE(IsValidUtf8(ToUtf8(input)));
    }

    // 6. Mixed input: "Voltage: 3.3V \xB15% \xD6" (± and Ö as raw CP1252)
    TEST_F(Utf8SanitizerTest, MixedInput_AsciiAndCp1252)
    {
        std::string input = "Voltage: 3.3V \xB1" "5% \xD6";
        EXPECT_FALSE(IsValidUtf8(input));
        std::string result = ToUtf8(input);
        EXPECT_TRUE(IsValidUtf8(result));
        // ± is 0xB1 -> U+00B1 -> 0xC2 0xB1
        // Ö is 0xD6 -> U+00D6 -> 0xC3 0x96
        std::string expected = "Voltage: 3.3V \xC2\xB1" "5% \xC3\x96";
        EXPECT_TRUE(BytesMatch(result, expected));
    }

    // 7. Invalid UTF-8 sequences (overlong 0xC0 0x80) -> treated as CP1252
    TEST_F(Utf8SanitizerTest, InvalidUtf8_Overlong)
    {
        std::string input = "\xC0\x80";
        EXPECT_FALSE(IsValidUtf8(input));
        std::string result = ToUtf8(input);
        EXPECT_TRUE(IsValidUtf8(result));
        // 0xC0 -> U+00C0 (À) -> 0xC3 0x80
        // 0x80 -> U+20AC (€) -> 0xE2 0x82 0xAC
        std::string expected = "\xC3\x80\xE2\x82\xAC";
        EXPECT_TRUE(BytesMatch(result, expected));
    }

    // 8. Invalid UTF-8 sequences (lone surrogate 0xED 0xA0 0x80) -> treated as CP1252
    TEST_F(Utf8SanitizerTest, InvalidUtf8_Surrogate)
    {
        std::string input = "\xED\xA0\x80";
        EXPECT_FALSE(IsValidUtf8(input));
        std::string result = ToUtf8(input);
        EXPECT_TRUE(IsValidUtf8(result));
    }

    // 9. Empty string in, empty string out
    TEST_F(Utf8SanitizerTest, EmptyString)
    {
        std::string input = "";
        EXPECT_TRUE(IsValidUtf8(input));
        EXPECT_EQ(ToUtf8(input), "");
    }

    // 10. Embedded NUL byte preserved
    TEST_F(Utf8SanitizerTest, EmbeddedNul)
    {
        // Create a string with embedded NUL: "A\0B"
        std::string input(3, '\0');
        input[0] = 'A';
        input[1] = '\0';
        input[2] = 'B';
        
        EXPECT_TRUE(IsValidUtf8(input)); // Already valid UTF-8
        std::string result = ToUtf8(input);
        EXPECT_EQ(result.size(), 3u);
        EXPECT_EQ(result[0], 'A');
        EXPECT_EQ(result[1], '\0');
        EXPECT_EQ(result[2], 'B');
    }

    // 11. SanitizeToUtf8 in-place modifies only when needed
    TEST_F(Utf8SanitizerTest, SanitizeToUtf8_InPlace)
    {
        std::string valid = "Hello";
        std::string original_valid = valid;
        SanitizeToUtf8(valid);
        EXPECT_EQ(valid, original_valid);

        std::string invalid = "\xD6";
        SanitizeToUtf8(invalid);
        EXPECT_TRUE(IsValidUtf8(invalid));
        EXPECT_EQ(invalid, "\xC3\x96");
    }

    // 12. Integration test: PropertyRecord with raw CP1252 value
    TEST_F(Utf8SanitizerTest, Integration_PropertyRecordSerialization)
    {
        Odb::Lib::FileModel::Design::PropertyRecord record;
        record.name = "Description";
        // Raw CP1252 for "Test Ö"
        record.value = "Test \xD6";

        auto pb = record.to_protobuf();
        ASSERT_NE(pb, nullptr);

        // Serialize to string
        std::string serialized = pb->SerializeAsString();
        EXPECT_FALSE(serialized.empty());

        // Re-parse into fresh message
        Odb::Lib::Protobuf::PropertyRecord parsed;
        bool parse_success = parsed.ParseFromString(serialized);
        EXPECT_TRUE(parse_success) << "Failed to parse serialized protobuf";

        // Verify decoded value matches expected UTF-8
        EXPECT_EQ(parsed.name(), "Description");
        // "Test " + Ö (0xC3 0x96)
        std::string expected_value = "Test \xC3\x96";
        EXPECT_EQ(parsed.value(), expected_value);
        EXPECT_TRUE(IsValidUtf8(parsed.value()));
    }
} // namespace Odb::Test
