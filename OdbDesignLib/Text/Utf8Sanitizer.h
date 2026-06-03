#pragma once

/**
 * @file Utf8Sanitizer.h
 * @brief UTF-8 validation and Windows-1252 to UTF-8 transcoding utilities.
 *
 * @why This exists because ODB++ design files exported from European EDA tools
 * often contain legacy 8-bit text encoded in Windows-1252 (CP1252) or ISO-8859-1.
 * When these raw bytes are copied verbatim into Protobuf `string` fields, they
 * produce invalid UTF-8 on the wire. Strict Protobuf clients (including .NET's
 * Google.Protobuf) refuse to parse such messages and throw
 * InvalidProtocolBufferException: "String is invalid UTF-8".
 *
 * This module provides:
 * - RFC 3629 compliant UTF-8 validation
 * - CP1252 → UTF-8 transcoding for invalid sequences
 * - Zero-allocation fast path for already-valid UTF-8
 *
 * @see https://github.com/nam20485/OdbDesign/docs/grpc/server-utf8-sanitization-prompt.md
 */

#include <cstddef>
#include <string>
#include <string_view>

namespace Odb::Lib::Text
{
    /**
     * @brief Validates that a byte sequence is valid UTF-8 per RFC 3629.
     *
     * Checks for:
     * - Correct continuation byte ranges
     * - No overlong encodings
     * - No surrogate halves (U+D800..U+DFFF)
     * - No code points above U+10FFFF
     *
     * @param data Pointer to the byte sequence
     * @param size Length in bytes
     * @return true if valid UTF-8, false otherwise
     */
    bool IsValidUtf8(const char* data, std::size_t size) noexcept;

    /**
     * @brief Validates that a string_view contains valid UTF-8.
     * @param s The string to validate
     * @return true if valid UTF-8, false otherwise
     */
    bool IsValidUtf8(std::string_view s) noexcept;

    /**
     * @brief Converts input to valid UTF-8.
     *
     * - If input is already valid UTF-8, returns a copy unchanged.
     * - Otherwise, decodes input as Windows-1252 and re-encodes as UTF-8.
     *
     * CP1252 differs from ISO-8859-1 only in the 0x80–0x9F range.
     * The five undefined CP1252 slots (0x81, 0x8D, 0x8F, 0x90, 0x9D)
     * are mapped to U+FFFD (REPLACEMENT CHARACTER).
     *
     * Never throws. Never returns invalid UTF-8.
     *
     * @param input The raw byte sequence from ODB++ files
     * @return A valid UTF-8 string
     */
    std::string ToUtf8(std::string_view input) noexcept;

    /**
     * @brief In-place convenience overload.
     *
     * Replaces the contents of @p s with ToUtf8(s) only if validation fails.
     * Avoids allocation when already valid UTF-8.
     *
     * @param s The string to sanitize
     */
    void SanitizeToUtf8(std::string& s) noexcept;

    /**
     * @brief Debug-only assertion to verify all string fields in a message are valid UTF-8.
     *
     * This function walks the protobuf message and asserts that every string field
     * contains valid UTF-8. It is only active in debug builds (#ifndef NDEBUG).
     *
     * @param msg The protobuf message to validate
     * @param msgName A descriptive name for the message type (used in assertion messages)
     */
    void AssertAllStringFieldsAreValidUtf8(const google::protobuf::Message& msg, std::string_view msgName);

} // namespace Odb::Lib::Text
