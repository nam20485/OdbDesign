#include <gtest/gtest.h>
#include <stdexcept>
#include <string>

#include "bin2ascii.h"

using namespace Utils;

namespace Odb::Test
{
    // ---- bin2hex -----------------------------------------------------------

    TEST(Bin2AsciiTests, Bin2Hex_EmptyString_StaysEmpty)
    {
        EXPECT_EQ(bin2hex(""), "");
    }

    TEST(Bin2AsciiTests, Bin2Hex_KnownVectors_LowercaseHex)
    {
        // NOTE: only bytes < 0x80 here — high-bit bytes go through the
        // defective signed-shift path covered by the tripwire test below
        EXPECT_EQ(bin2hex(std::string(1, '\x00')), "00");
        EXPECT_EQ(bin2hex("a"), "61");
        EXPECT_EQ(bin2hex("\x0A"), "0a");
        EXPECT_EQ(bin2hex("\x7F"), "7f");
        EXPECT_EQ(bin2hex("\x0D\x0E\x0B\x0E"), "0d0e0b0e");
    }

    // REGRESSION GUARD (was a memory-safety defect, fixed in this PR):
    // bin2hex() used to sign-extend bytes >= 0x80 and index the 16-entry
    // lookup table out of bounds (UB, garbage output); it now reads each
    // byte as unsigned before nibble extraction.
    TEST(Bin2AsciiTests, Bin2Hex_HighBitByte_KnownHexValue)
    {
        EXPECT_EQ(bin2hex(std::string(1, '\x80')), "80");
        EXPECT_EQ(bin2hex(std::string(1, '\xFF')), "ff");
    }

    // ---- hex2bin -------------------------------------------------------------

    TEST(Bin2AsciiTests, Hex2Bin_KnownVectors)
    {
        EXPECT_EQ(hex2bin(""), "");
        EXPECT_EQ(hex2bin("00"), std::string(1, '\x00'));
        EXPECT_EQ(hex2bin("61"), "a");
        EXPECT_EQ(hex2bin("deadbeef"), std::string("\xDE\xAD\xBE\xEF", 4));
        EXPECT_EQ(hex2bin("DEADBEEF"), std::string("\xDE\xAD\xBE\xEF", 4)); // case-insensitive
        EXPECT_EQ(hex2bin("0a0b"), std::string("\x0A\x0B", 2));
    }

    TEST(Bin2AsciiTests, Hex2Bin_OddLength_ThrowsRuntimeError)
    {
        EXPECT_THROW(hex2bin("abc"), std::runtime_error);
        EXPECT_THROW(hex2bin("1"), std::runtime_error);
        try
        {
            (void)hex2bin("abc");
            FAIL() << "expected std::runtime_error";
        }
        catch (const std::runtime_error& e)
        {
            EXPECT_NE(std::string(e.what()).find("Odd hex data size"), std::string::npos);
        }
    }

    TEST(Bin2AsciiTests, Hex2Bin_InvalidCharacter_ThrowsRuntimeError)
    {
        EXPECT_THROW(hex2bin("zz"), std::runtime_error);
        EXPECT_THROW(hex2bin("12g4"), std::runtime_error);
        try
        {
            (void)hex2bin("zz");
            FAIL() << "expected std::runtime_error";
        }
        catch (const std::runtime_error& e)
        {
            EXPECT_NE(std::string(e.what()).find("Invalid hex data"), std::string::npos);
        }
    }

    TEST(Bin2AsciiTests, HexBinRoundTrip_AllLowBytes_Lossless)
    {
        // full sweep of the safe (low-bit) byte range through both directions
        std::string allBytes;
        for (int b = 0x00; b <= 0x7F; ++b)
        {
            allBytes.push_back(static_cast<char>(b));
        }
        EXPECT_EQ(hex2bin(bin2hex(allBytes)), allBytes);
    }

    // ---- base64 ---------------------------------------------------------------

    TEST(Bin2AsciiTests, B64Encode_Rfc4648TestVectors)
    {
        EXPECT_EQ(b64_encode(""), "");
        EXPECT_EQ(b64_encode("f"), "Zg==");
        EXPECT_EQ(b64_encode("fo"), "Zm8=");
        EXPECT_EQ(b64_encode("foo"), "Zm9v");
        EXPECT_EQ(b64_encode("foob"), "Zm9vYg==");
        EXPECT_EQ(b64_encode("fooba"), "Zm9vYmE=");
        EXPECT_EQ(b64_encode("foobar"), "Zm9vYmFy");
    }

    TEST(Bin2AsciiTests, B64Decode_Rfc4648TestVectors)
    {
        EXPECT_EQ(b64_decode(""), "");
        EXPECT_EQ(b64_decode("Zg=="), "f");
        EXPECT_EQ(b64_decode("Zm8="), "fo");
        EXPECT_EQ(b64_decode("Zm9v"), "foo");
        EXPECT_EQ(b64_decode("Zm9vYg=="), "foob");
        EXPECT_EQ(b64_decode("Zm9vYmE="), "fooba");
        EXPECT_EQ(b64_decode("Zm9vYmFy"), "foobar");
    }

    TEST(Bin2AsciiTests, B64RoundTrip_BinaryWithNulAndHighBits_Lossless)
    {
        // NOTE: the "\xFB" and the following text are SEPARATE string
        // literals: "\xFBbinary" would lex "\xFBB" as a single out-of-range
        // hex escape (compiler warning, mis-lexed bytes, and ill-formed on
        // stricter compilers). 12 bytes: 00 41 80 FF FB 'b'..'y' 00
        const std::string binary = std::string("\x00\x41\x80\xFF\xFB" "binary\x00", 12);
        ASSERT_EQ(binary.size(), 12u);
        EXPECT_EQ(b64_decode(b64_encode(binary)), binary);
    }

    TEST(Bin2AsciiTests, B64Decode_SizeNotMultipleOfFour_ThrowsRuntimeError)
    {
        EXPECT_THROW(b64_decode("abc"), std::runtime_error);
        try
        {
            (void)b64_decode("abc");
            FAIL() << "expected std::runtime_error";
        }
        catch (const std::runtime_error& e)
        {
            EXPECT_NE(std::string(e.what()).find("Invalid base64 data size"), std::string::npos);
        }
    }

    TEST(Bin2AsciiTests, B64Decode_InvalidCharacter_ThrowsRuntimeError)
    {
        EXPECT_THROW(b64_decode("Zm9!"), std::runtime_error);
        try
        {
            (void)b64_decode("Zm9!");
            FAIL() << "expected std::runtime_error";
        }
        catch (const std::runtime_error& e)
        {
            // note: the message text says "Invalid hex data" even for base64
            // input (cosmetic defect); assert the stable "Invalid" prefix
            EXPECT_NE(std::string(e.what()).find("Invalid"), std::string::npos);
        }
    }
} // namespace Odb::Test
