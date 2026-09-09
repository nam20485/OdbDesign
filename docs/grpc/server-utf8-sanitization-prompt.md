# Server UTF-8 Sanitization Prompt

Copy the prompt below into your CLion AI chat for the server-side fix.

---

## Task: Sanitize ODB++ text to valid UTF-8 before Protobuf serialization

### Background / Why

This C++ server loads ODB++ design data and exposes it via gRPC using Protobuf messages defined in `Odb.Lib.Protobuf.*` (e.g. `PropertyRecord`, `ComponentRecord`, `ComponentsFile`, `LayerDirectory`, `StepDirectory`, `FileArchive`, `Design`).

When the server serializes a `Design` for the design `designodb_rigidflex`, the C++ Protobuf runtime logs the following warning, twice per request:

```
E0000 ... wire_format_lite.cc:610] String field 'Odb.Lib.Protobuf.PropertyRecord.value'
contains invalid UTF-8 data when serializing a protocol buffer.
Use the 'bytes' type if you intend to send raw bytes.
```

The Protobuf `string` type is, per the Protobuf spec, **required to be valid UTF-8**. The C++ runtime only *warns* on invalid UTF-8 during serialization — it still puts the raw bytes on the wire. However, **strict clients (including .NET / Google.Protobuf for C#) refuse to parse `string` fields whose bytes are not valid UTF-8** and throw `InvalidProtocolBufferException: String is invalid UTF-8` (inner `DecoderFallbackException: Unable to translate bytes [D6] at index 1 ...`). This causes the gRPC call to fail on the client with `StatusCode.Unavailable`, breaking the client's ability to load that design.

The byte `0xD6` (and other bytes in the `0x80–0xFF` range) showing up bare strongly indicates the source ODB++ files contain **legacy 8-bit text encoded in Windows-1252 / ISO-8859-1** (very common for ODB++ exports from European EDA tools; `0xD6` is `Ö` in CP1252/Latin-1). These bytes are being copied verbatim into the Protobuf `string` field `PropertyRecord.value` without transcoding.

### Goal

**Fix this on the server side** by sanitizing all text read from ODB++ source files into valid UTF-8 *before* it is assigned to any Protobuf `string` field. Do not change the `.proto` schema (keep `PropertyRecord.value` as `string`). After the fix:

1. The C++ Protobuf "invalid UTF-8" warning must no longer appear for any field, for any design.
2. .NET clients must be able to successfully deserialize `Design` messages that previously failed.
3. Characters originally encoded as CP1252/Latin-1 (e.g. `Ö`, `ä`, `ß`, `é`, `°`, `µ`, `±`) must round-trip to readable Unicode characters on the client, not mojibake.

### What I need you to do

Please do the following in order. Ask me for clarification if anything is ambiguous — do **not** guess silently.

#### 1. Reconnaissance

- Locate the file `OdbDesignServiceImpl.cpp` (path mentioned in the server log) and the gRPC service implementation around the `GetDesign` RPC at lines ~122 and ~132.
- Locate the `.proto` files that define `Odb.Lib.Protobuf.PropertyRecord`, `ComponentRecord`, `ComponentsFile`, `LayerDirectory`, `StepDirectory`, `FileArchive`, and `Design`. Confirm field types.
- Identify **all** code paths that populate Protobuf `string` fields from ODB++ source data. Pay particular attention to:
    - `PropertyRecord.value` (confirmed offender)
    - `PropertyRecord.name`
    - `ComponentRecord` fields (part name, package ref, etc.)
    - Any attribute / property maps in `ComponentsFile`, `LayerDirectory`, `StepDirectory`, `FileArchive`, `Design`
    - Any other `string` field whose content originates from a parsed ODB++ file.
- Identify the ODB++ parser / loader layer and where raw bytes are read from disk (file streams, line buffers, tokenizers). Report back the namespaces/classes involved and the typical call chain from "read file" → "set Protobuf field".

Summarize findings to me before making changes.

#### 2. Design the sanitization layer

Implement a small, well-tested utility, e.g. in a new pair of files:

- `src/Odb/Lib/Text/Utf8Sanitizer.h`
- `src/Odb/Lib/Text/Utf8Sanitizer.cpp`

(Adjust the path to match the project's existing conventions — discover and follow them.)

Public API:

```cpp
namespace Odb::Lib::Text {

// Returns true iff [data, data+size) is a valid UTF-8 byte sequence
// (per RFC 3629: no overlong forms, no surrogates U+D800..U+DFFF,
// no code points > U+10FFFF).
bool IsValidUtf8(const char* data, std::size_t size) noexcept;
bool IsValidUtf8(std::string_view s) noexcept;

// Returns a UTF-8 std::string:
//   - If `input` is already valid UTF-8, returns it unchanged (copy).
//   - Otherwise, decodes `input` as Windows-1252 and re-encodes as UTF-8.
// Never throws. Never returns invalid UTF-8.
std::string ToUtf8(std::string_view input);

// In-place convenience overload: replaces `s` with ToUtf8(s) if needed.
void SanitizeToUtf8(std::string& s);

} // namespace Odb::Lib::Text
```

Implementation requirements:

- **No external dependencies if avoidable.** ICU/iconv are overkill here — a hand-written CP1252→UTF-8 table is ~30 lines and sufficient. CP1252 differs from ISO-8859-1 only in the `0x80–0x9F` range; use the official CP1252 mapping (including the handful of undefined slots `0x81, 0x8D, 0x8F, 0x90, 0x9D`, which should be mapped to U+FFFD).
- `IsValidUtf8` must implement the full RFC 3629 state machine (validate continuation byte ranges, reject overlongs, reject surrogates, reject > U+10FFFF). Do **not** rely on `std::mbrtoc16` / locale-dependent functions.
- `ToUtf8` must be allocation-light on the common case (already-valid UTF-8): a single validation pass and a copy, no second allocation.
- All functions must be `noexcept` where the signature allows, and must handle embedded NULs.
- Add Doxygen-style comments explaining *why* this exists (link conceptually to the gRPC client failure).

#### 3. Wire the sanitizer into the Protobuf-building code

For every site identified in step 1 that assigns ODB++-sourced text into a Protobuf `string` field, route the value through `Odb::Lib::Text::ToUtf8(...)` first. Examples (adapt to actual code):

```cpp
// Before
record->set_value(rawValueFromOdb);

// After
record->set_value(Odb::Lib::Text::ToUtf8(rawValueFromOdb));
```

Guidance:

- Prefer fixing this at the **lowest possible layer** — ideally where the ODB++ parser produces `std::string` tokens — so that *all* downstream consumers (Protobuf serializers, logging, anything else) get clean UTF-8 automatically. If that layer is shared with code that legitimately needs raw bytes, fix at the Protobuf-assignment boundary instead.
- Do **not** sanitize fields whose contents are already guaranteed UTF-8 or pure ASCII (e.g. enum-like tokens, numeric strings, file paths that originate from `std::filesystem`) — sanitizing them is harmless but adds noise. Use judgment; when in doubt, sanitize.
- Do **not** change the `.proto` schema. Do **not** change field types to `bytes`.

#### 4. Tests

Add unit tests (using whatever framework the project already uses — discover it; GoogleTest is most likely). Cover:

- ASCII passes through unchanged.
- Valid multi-byte UTF-8 (e.g. `"Ö"` as `0xC3 0x96`, `"€"` as `0xE2 0x82 0xAC`, an emoji as 4-byte UTF-8) passes through unchanged byte-for-byte.
- Single CP1252 byte `0xD6` → UTF-8 `0xC3 0x96` (`Ö`).
- Single CP1252 byte `0x80` → UTF-8 for `€` (U+20AC = `0xE2 0x82 0xAC`).
- Undefined CP1252 byte `0x81` → U+FFFD (`0xEF 0xBF 0xBD`).
- Mixed input: `"Voltage: 3.3V ±5% Ö"` where the `±` and `Ö` are raw CP1252 bytes — output is fully valid UTF-8.
- Invalid UTF-8 sequences (overlong `0xC0 0x80`, lone surrogate `0xED 0xA0 0x80`, truncated multi-byte) → treated as CP1252 and transcoded (no exception, no crash).
- Empty string in, empty string out.
- Embedded NUL byte preserved.
- `IsValidUtf8` returns the correct boolean for each of the above.

Add at least one **integration-style test** that builds a `PropertyRecord` whose `value` is a raw CP1252 string, runs it through whatever production helper now sets the field, serializes the parent message with `SerializeAsString()`, and asserts that re-parsing it into a fresh message succeeds **and** that the resulting `value` decodes as the expected Unicode characters (i.e. compare bytes to the expected UTF-8 byte sequence).

#### 5. Verification

- Build the project in the same configuration shown in the log path (`obj/unix/Release/net10.0` suggests a Linux Release build).
- Run all existing tests; none should regress.
- Run the new tests; all should pass.
- Manually exercise the `GetDesign` RPC for design name `designodb_rigidflex` (the one currently failing) and confirm:
    - **Zero** `wire_format_lite.cc:610 ... contains invalid UTF-8 data` warnings in the server log.
    - The response is byte-identical-in-meaning to before (i.e. the client now sees properly decoded `Ö`/etc., not garbage).
- If you can, add a debug-only assertion (`#ifndef NDEBUG`) right before `SerializeToString` / `SerializeToOstream` of the top-level `Design` message that walks the message and asserts every `string` field is valid UTF-8. This makes future regressions impossible to miss. Keep it cheap or gated behind a flag if the assertion is expensive on 116 MB messages.

#### 6. Deliverables

When done, summarize:

1. List of files added.
2. List of files modified, with one-line rationale each.
3. Test results (paste output).
4. The exact `git diff --stat`.
5. Any sites you found that *might* feed non-UTF-8 into Protobuf `string` fields but you did **not** sanitize, and why.
6. Any follow-ups you recommend (e.g. extending sanitization to logging, exposing the original encoding as metadata, etc.).

### Hard constraints / things NOT to do

- Do **not** change the `.proto` schema or any field type.
- Do **not** silently drop characters or replace whole strings with placeholders. Each input byte must produce a defined Unicode result (CP1252 → its Unicode codepoint, or U+FFFD for the five undefined slots).
- Do **not** introduce a new heavy dependency (ICU, Boost.Locale, iconv) just for this. A static CP1252 table is sufficient.
- Do **not** use locale-dependent C functions (`mbstowcs`, `setlocale`, etc.).
- Do **not** sanitize bytes that are already valid UTF-8 by re-decoding them as CP1252 — that would corrupt correctly-encoded data. The `IsValidUtf8` fast path is mandatory.
- Do **not** suppress the Protobuf warning by disabling logging. Fix the data.

### Context recap (one-liner)

> Some ODB++ source files contain Windows-1252-encoded text. The server currently copies those raw bytes into Protobuf `string` fields, producing invalid UTF-8 on the wire, which breaks strict clients. Fix it by transcoding to UTF-8 at the boundary so the schema stays clean and all clients work.

Please start with **step 1 (Reconnaissance)** and report findings before writing code.
