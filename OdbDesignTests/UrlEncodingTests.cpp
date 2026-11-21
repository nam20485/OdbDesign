#include <gtest/gtest.h>
#include <iomanip>
#include <sstream>
#include <string>

#include "UrlEncoding.h"

using ::Utils::UrlEncoding;

namespace Odb::Test
{
    // UrlEncoding implements application/x-www-form-urlencoded semantics:
    //   - alphanumerics pass through
    //   - space -> '+'
    //   - everything else -> '%' followed by TWO UPPERCASE hex digits
    // Encode and decode are each validated against independently known
    // expected byte strings below; round-trip alone is not sufficient.

    TEST(UrlEncodingTests, Encode_Alphanumeric_Passthrough)
    {
        EXPECT_EQ(UrlEncoding::encode("abcXYZ019"), "abcXYZ019");
    }

    TEST(UrlEncodingTests, Encode_EmptyString_StaysEmpty)
    {
        EXPECT_EQ(UrlEncoding::encode(""), "");
    }

    TEST(UrlEncodingTests, Encode_Space_BecomesPlus)
    {
        EXPECT_EQ(UrlEncoding::encode("a b c"), "a+b+c");
        EXPECT_EQ(UrlEncoding::encode(" "), "+");
    }

    TEST(UrlEncodingTests, Encode_ReservedChars_TwoUppercaseHexDigits)
    {
        EXPECT_EQ(UrlEncoding::encode("%"), "%25");
        EXPECT_EQ(UrlEncoding::encode("&"), "%26");
        EXPECT_EQ(UrlEncoding::encode("="), "%3D");
        EXPECT_EQ(UrlEncoding::encode("+"), "%2B");
        EXPECT_EQ(UrlEncoding::encode("?"), "%3F");
        EXPECT_EQ(UrlEncoding::encode("#"), "%23");
        EXPECT_EQ(UrlEncoding::encode("/"), "%2F");
        EXPECT_EQ(UrlEncoding::encode(":"), "%3A");
        EXPECT_EQ(UrlEncoding::encode("~"), "%7E");
    }

    TEST(UrlEncodingTests, Encode_ControlChars_KnownHex)
    {
        EXPECT_EQ(UrlEncoding::encode("\t"), "%09");
        EXPECT_EQ(UrlEncoding::encode("\n"), "%0A");
        EXPECT_EQ(UrlEncoding::encode("\r"), "%0D");
        // NUL is encoded like any other byte (never truncates the string)
        EXPECT_EQ(UrlEncoding::encode(std::string(1, '\0')), "%00");
    }

    TEST(UrlEncodingTests, Encode_MixedQuery_KnownVector)
    {
        EXPECT_EQ(UrlEncoding::encode("name=John Smith&age=30"),
                  "name%3DJohn+Smith%26age%3D30");
    }

    // KNOWN DEFECT TRIPWIRE (currently FAILS): encode() computes the hex
    // digits from a SIGNED char (Utils/UrlEncoding.cpp:45: `c >> 4` and
    // `c % 16` on `const char& c`), so bytes >= 0x80 encode as garbage
    // instead of their true hex value (e.g. 0xE9 currently yields "%50"
    // on Linux rather than "%E9"). This test asserts the CORRECT
    // behavior; fixing the implementation should make it pass.
    TEST(UrlEncodingTests, Encode_NonAsciiByte_TrueHexValue)
    {
        EXPECT_EQ(UrlEncoding::encode(std::string(1, '\xE9')), "%E9");
        EXPECT_EQ(UrlEncoding::encode(std::string(1, '\x80')), "%80");
    }

    TEST(UrlEncodingTests, Decode_Plus_BecomesSpace)
    {
        EXPECT_EQ(UrlEncoding::decode("+"), " ");
        EXPECT_EQ(UrlEncoding::decode("a+b"), "a b");
    }

    TEST(UrlEncodingTests, Decode_KnownVectors_UpperAndLowercaseHex)
    {
        EXPECT_EQ(UrlEncoding::decode("%41"), "A");
        EXPECT_EQ(UrlEncoding::decode("%3D"), "=");
        EXPECT_EQ(UrlEncoding::decode("%3d"), "=");   // lowercase hex accepted
        EXPECT_EQ(UrlEncoding::decode("%2F"), "/");
        EXPECT_EQ(UrlEncoding::decode("%25"), "%");
        EXPECT_EQ(UrlEncoding::decode("%7e"), "~");
        EXPECT_EQ(UrlEncoding::decode("name%3DJohn+Smith"), "name=John Smith");
    }

    TEST(UrlEncodingTests, Decode_NonAsciiByte_KnownByteValue)
    {
        // decode() handles high-bit bytes correctly (unlike encode)
        EXPECT_EQ(UrlEncoding::decode("%E9"), std::string(1, '\xE9'));
        EXPECT_EQ(UrlEncoding::decode("%80"), std::string(1, '\x80'));
    }

    TEST(UrlEncodingTests, Decode_TruncatedEscape_Passthrough)
    {
        // '%' without two following hex characters passes through unchanged
        EXPECT_EQ(UrlEncoding::decode("%"), "%");
        EXPECT_EQ(UrlEncoding::decode("100%"), "100%");
        EXPECT_EQ(UrlEncoding::decode("%4"), "%4");
        EXPECT_EQ(UrlEncoding::decode("a%2"), "a%2");
    }

    TEST(UrlEncodingTests, Decode_InvalidHexDigits_DecodedAsZeroBytes)
    {
        // Documented actual policy: from_hex() maps any non-hex character to
        // 0, so malformed escapes decode silently instead of failing.
        // "%2G": hi=2, lo=0 -> byte 0x20 (space)
        EXPECT_EQ(UrlEncoding::decode("%2G"), " ");
        // "%ZZ": both digits invalid -> byte 0x00 (embedded NUL)
        EXPECT_EQ(UrlEncoding::decode("%ZZ"), std::string(1, '\0'));
    }

    TEST(UrlEncodingTests, Decode_CanEmitEmbeddedNul)
    {
        std::string expected = std::string("a\0b", 3);
        EXPECT_EQ(UrlEncoding::decode("a%00b"), expected);
        EXPECT_EQ(UrlEncoding::decode("a%00b").size(), 3u);
    }

    TEST(UrlEncodingTests, RoundTrip_DecodeOfEncode_AllAsciiBytes)
    {
        // property check over the full ASCII range (the known vectors above
        // carry the correctness burden; this just confirms no byte is lost)
        for (int c = 0; c <= 0x7F; ++c)
        {
            std::string in(1, static_cast<char>(c));
            std::string encoded = UrlEncoding::encode(in);
            EXPECT_EQ(UrlEncoding::decode(encoded), in)
                << "byte 0x" << std::hex << c
                << " encoded as '" << encoded << "'";
        }
    }
} // namespace Odb::Test
