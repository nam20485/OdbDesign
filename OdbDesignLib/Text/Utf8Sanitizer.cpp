/**
 * @file Utf8Sanitizer.cpp
 * @brief Implementation of UTF-8 validation and CP1252→UTF-8 transcoding.
 *
 * The CP1252→Unicode mapping table is derived from the official Microsoft mapping:
 * https://www.unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP1252.TXT
 *
 * Undefined slots at 0x81, 0x8D, 0x8F, 0x90, 0x9D map to U+FFFD.
 */

#include "Utf8Sanitizer.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace Odb::Lib::Text
{
    namespace
    {
        // clang-format off
        /**
         * @brief Windows-1252 to Unicode code point mapping table.
         * 
         * Index is the CP1252 byte value (0x00–0xFF).
         * Value is the corresponding Unicode code point.
         * 
         * - 0x00–0x7F: ASCII (identity mapping)
         * - 0x80–0x9F: CP1252 special characters (differs from ISO-8859-1)
         * - 0xA0–0xFF: ISO-8859-1 upper half (mostly Latin-1 supplement)
         */
        constexpr uint32_t kCp1252ToUnicode[256] = {
            // 0x00-0x7F: ASCII (identity)
            0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
            0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F,
            0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017,
            0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x001E, 0x001F,
            0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
            0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
            0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
            0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
            0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
            0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
            0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
            0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
            0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
            0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
            0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
            0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x007F,

            // 0x80-0x9F: CP1252 special characters
            0x20AC, // 0x80: EURO SIGN
            0xFFFD, // 0x81: UNDEFINED → REPLACEMENT CHARACTER
            0x201A, // 0x82: SINGLE LOW-9 QUOTATION MARK
            0x0192, // 0x83: LATIN SMALL LETTER F WITH HOOK
            0x201E, // 0x84: DOUBLE LOW-9 QUOTATION MARK
            0x2026, // 0x85: HORIZONTAL ELLIPSIS
            0x2020, // 0x86: DAGGER
            0x2021, // 0x87: DOUBLE DAGGER
            0x02C6, // 0x88: MODIFIER LETTER CIRCUMFLEX ACCENT
            0x2030, // 0x89: PER MILLE SIGN
            0x0160, // 0x8A: LATIN CAPITAL LETTER S WITH CARON
            0x2039, // 0x8B: SINGLE LEFT-POINTING ANGLE QUOTATION MARK
            0x0152, // 0x8C: LATIN CAPITAL LIGATURE OE
            0xFFFD, // 0x8D: UNDEFINED → REPLACEMENT CHARACTER
            0x017D, // 0x8E: LATIN CAPITAL LETTER Z WITH CARON
            0xFFFD, // 0x8F: UNDEFINED → REPLACEMENT CHARACTER
            0xFFFD, // 0x90: UNDEFINED → REPLACEMENT CHARACTER
            0x2018, // 0x91: LEFT SINGLE QUOTATION MARK
            0x2019, // 0x92: RIGHT SINGLE QUOTATION MARK
            0x201C, // 0x93: LEFT DOUBLE QUOTATION MARK
            0x201D, // 0x94: RIGHT DOUBLE QUOTATION MARK
            0x2022, // 0x95: BULLET
            0x2013, // 0x96: EN DASH
            0x2014, // 0x97: EM DASH
            0x02DC, // 0x98: SMALL TILDE
            0x2122, // 0x99: TRADE MARK SIGN
            0x0161, // 0x9A: LATIN SMALL LETTER S WITH CARON
            0x203A, // 0x9B: SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
            0x0153, // 0x9C: LATIN SMALL LIGATURE OE
            0xFFFD, // 0x9D: UNDEFINED → REPLACEMENT CHARACTER
            0x017E, // 0x9E: LATIN SMALL LETTER Z WITH CARON
            0x0178, // 0x9F: LATIN CAPITAL LETTER Y WITH DIAERESIS

            // 0xA0-0xFF: ISO-8859-1 upper half (Latin-1 Supplement)
            0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7,
            0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
            0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
            0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
            0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7,
            0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
            0x00D0, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x00D7,
            0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x00DF,
            0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7,
            0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
            0x00F0, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x00F7,
            0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0x00FF
        };
        // clang-format on

        /**
         * @brief Encodes a Unicode code point as UTF-8.
         * @param codepoint The Unicode code point (U+0000 to U+10FFFF)
         * @param output Pointer to output buffer (must have space for 4 bytes)
         * @return Number of bytes written (1-4)
         */
        inline int EncodeUtf8Codepoint(uint32_t codepoint, char* output) noexcept
        {
            if (codepoint <= 0x7F)
            {
                output[0] = static_cast<char>(codepoint);
                return 1;
            }
            else if (codepoint <= 0x7FF)
            {
                output[0] = static_cast<char>(0xC0 | (codepoint >> 6));
                output[1] = static_cast<char>(0x80 | (codepoint & 0x3F));
                return 2;
            }
            else if (codepoint <= 0xFFFF)
            {
                output[0] = static_cast<char>(0xE0 | (codepoint >> 12));
                output[1] = static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                output[2] = static_cast<char>(0x80 | (codepoint & 0x3F));
                return 3;
            }
            else
            {
                output[0] = static_cast<char>(0xF0 | (codepoint >> 18));
                output[1] = static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
                output[2] = static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                output[3] = static_cast<char>(0x80 | (codepoint & 0x3F));
                return 4;
            }
        }

#ifndef NDEBUG
        /**
         * @brief Reports an invalid UTF-8 value found in a message field, then aborts.
         *
         * Prints the field path and a hex-escaped sample of the offending value to
         * stderr. Portable replacement for glibc's __assert_fail (not available on
         * MSVC/Windows).
         */
        [[noreturn]] void FailOnInvalidUtf8(const std::string& fieldPath, const std::string& value)
        {
            std::fprintf(stderr, "Utf8Sanitizer assertion failed: invalid UTF-8 in %s: \"", fieldPath.c_str());
            const std::size_t sampleLen = value.size() < 32 ? value.size() : 32;
            for (std::size_t i = 0; i < sampleLen; ++i)
            {
                const unsigned char c = static_cast<unsigned char>(value[i]);
                if (c >= 0x20 && c < 0x7F)
                {
                    std::fputc(c, stderr);
                }
                else
                {
                    std::fprintf(stderr, "\\x%02X", c);
                }
            }
            if (value.size() > sampleLen)
            {
                std::fprintf(stderr, "...");
            }
            std::fprintf(stderr, "\"\n");
            std::abort();
        }

        void AssertMessageStringsAreValidUtf8(const google::protobuf::Message& msg, std::string& fieldPath);

        /**
         * @brief Validates a single string value, aborting (in debug builds) if invalid.
         */
        void AssertStringIsValidUtf8(const std::string& value, const std::string& fieldPath)
        {
            if (!value.empty() && !IsValidUtf8(value))
            {
                FailOnInvalidUtf8(fieldPath, value);
            }
        }

        /**
         * @brief Recursively validates every `string` field reachable from @p msg.
         *
         * Handles singular strings, repeated strings, and strings nested in
         * sub-messages. Map fields are covered implicitly: protobuf exposes a
         * map<K,V> field as a repeated message of synthetic MapEntry messages,
         * so recursing into repeated message fields validates map keys and values.
         * `bytes` fields are skipped (arbitrary binary data is allowed there).
         *
         * @param msg The message to validate
         * @param fieldPath Dot-separated path of the current message, extended as we recurse
         */
        void AssertMessageStringsAreValidUtf8(const google::protobuf::Message& msg, std::string& fieldPath)
        {
            const google::protobuf::Descriptor* descriptor = msg.GetDescriptor();
            const google::protobuf::Reflection* reflection = msg.GetReflection();

            for (int i = 0; i < descriptor->field_count(); ++i)
            {
                const google::protobuf::FieldDescriptor* field = descriptor->field(i);
                const std::size_t basePathLen = fieldPath.size();
                fieldPath.append(".").append(field->name());

                if (field->is_repeated())
                {
                    const int count = reflection->FieldSize(msg, field);
                    if (field->type() == google::protobuf::FieldDescriptor::TYPE_STRING)
                    {
                        for (int j = 0; j < count; ++j)
                        {
                            AssertStringIsValidUtf8(reflection->GetRepeatedString(msg, field, j), fieldPath);
                        }
                    }
                    else if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
                    {
                        for (int j = 0; j < count; ++j)
                        {
                            const std::size_t indexPathLen = fieldPath.size();
                            fieldPath.append("[").append(std::to_string(j)).append("]");
                            AssertMessageStringsAreValidUtf8(reflection->GetRepeatedMessage(msg, field, j), fieldPath);
                            fieldPath.resize(indexPathLen);
                        }
                    }
                }
                else
                {
                    // Skip unset fields when presence tracking is available
                    if (field->has_presence() && !reflection->HasField(msg, field))
                    {
                        fieldPath.resize(basePathLen);
                        continue;
                    }

                    if (field->type() == google::protobuf::FieldDescriptor::TYPE_STRING)
                    {
                        AssertStringIsValidUtf8(reflection->GetString(msg, field), fieldPath);
                    }
                    else if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
                    {
                        AssertMessageStringsAreValidUtf8(reflection->GetMessage(msg, field), fieldPath);
                    }
                }

                fieldPath.resize(basePathLen);
            }
        }
#endif // !NDEBUG
    } // anonymous namespace

    bool IsValidUtf8(const char* data, std::size_t size) noexcept
    {
        if (data == nullptr && size > 0) return false;

        const auto* bytes = reinterpret_cast<const uint8_t*>(data);
        std::size_t i = 0;

        while (i < size)
        {
            const uint8_t byte = bytes[i];

            // Single byte: 0xxxxxxx
            if (byte <= 0x7F)
            {
                ++i;
                continue;
            }

            // Two bytes: 110xxxxx 10xxxxxx
            if ((byte & 0xE0) == 0xC0)
            {
                if (i + 1 >= size) return false;
                if ((bytes[i + 1] & 0xC0) != 0x80) return false;

                // Check for overlong encoding
                const uint32_t codepoint = ((byte & 0x1F) << 6) | (bytes[i + 1] & 0x3F);
                if (codepoint < 0x80) return false;

                i += 2;
            }
            // Three bytes: 1110xxxx 10xxxxxx 10xxxxxx
            else if ((byte & 0xF0) == 0xE0)
            {
                if (i + 2 >= size) return false;
                if ((bytes[i + 1] & 0xC0) != 0x80 || (bytes[i + 2] & 0xC0) != 0x80)
                    return false;

                const uint32_t codepoint =
                    ((byte & 0x0F) << 12) | ((bytes[i + 1] & 0x3F) << 6) | (bytes[i + 2] & 0x3F);

                // Check for overlong encoding
                if (codepoint < 0x800) return false;

                // Reject surrogate halves U+D800..U+DFFF
                if (codepoint >= 0xD800 && codepoint <= 0xDFFF) return false;

                i += 3;
            }
            // Four bytes: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
            else if ((byte & 0xF8) == 0xF0)
            {
                if (i + 3 >= size) return false;
                if ((bytes[i + 1] & 0xC0) != 0x80 || (bytes[i + 2] & 0xC0) != 0x80 ||
                    (bytes[i + 3] & 0xC0) != 0x80)
                    return false;

                const uint32_t codepoint =
                    ((byte & 0x07) << 18) | ((bytes[i + 1] & 0x3F) << 12) |
                    ((bytes[i + 2] & 0x3F) << 6) | (bytes[i + 3] & 0x3F);

                // Check for overlong encoding
                if (codepoint < 0x10000) return false;

                // Reject code points above U+10FFFF
                if (codepoint > 0x10FFFF) return false;

                i += 4;
            }
            else
            {
                // Invalid leading byte
                return false;
            }
        }

        return true;
    }

    bool IsValidUtf8(std::string_view s) noexcept
    {
        return IsValidUtf8(s.data(), s.size());
    }

    std::string ToUtf8(std::string_view input)
    {
        // Fast path: already valid UTF-8
        if (IsValidUtf8(input))
        {
            return std::string(input);
        }

        // Slow path: transcode from CP1252
        // Worst case: each byte becomes 3 UTF-8 bytes (CP1252 0x80-0x9F map to
        // BMP code points that encode as 3-byte UTF-8 sequences)
        std::string result;
        result.reserve(input.size() * 3);

        char utf8Buf[4];
        for (unsigned char byte : input)
        {
            const uint32_t codepoint = kCp1252ToUnicode[byte];
            const int len = EncodeUtf8Codepoint(codepoint, utf8Buf);
            result.append(utf8Buf, len);
        }

        return result;
    }

    void SanitizeToUtf8(std::string& s)
    {
        if (!IsValidUtf8(s))
        {
            s = ToUtf8(s);
        }
    }

    void AssertAllStringFieldsAreValidUtf8(const google::protobuf::Message& msg, std::string_view msgName)
    {
#ifndef NDEBUG
        std::string fieldPath(msgName);
        AssertMessageStringsAreValidUtf8(msg, fieldPath);
#else
        (void)msg;
        (void)msgName;
#endif
    }

} // namespace Odb::Lib::Text
