#pragma once

// Strong ETag support for the per-design HTTP data endpoints (M1.3).
//
// An ETag binds the design name, the design archive file's mtime and size, and
// the endpoint path with an FNV-1a 64-bit hash, rendered as a quoted lowercase
// hex string ("a1b2c3d4e5f60718").
//
// Invalidation contract: because the tag includes the archive file's mtime and
// size, replacing or re-uploading a design archive (DesignCache reload,
// FileUploadController overwrite, POST of a new archive file) changes the tag
// with no explicit invalidation step — a client revalidating with the stale
// tag's If-None-Match no longer matches and receives a fresh 200.
//
// Gzip interplay: the tag is the strong validator of the UNCOMPRESSED
// representation. It is computed before Crow serializes the body, hence also
// before Crow compresses it for the wire, so it does not vary with the
// content-coding (a strong variant of the uncompressed representation).

#include <cctype>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>
#include <system_error>

namespace Utils
{
	constexpr inline std::uint64_t FNV1A64_OFFSET_BASIS = 14695981039346656037ull;
	constexpr inline std::uint64_t FNV1A64_PRIME = 1099511628211ull;

	// FNV-1a 64-bit over sizeBytes bytes at data, continuing from hash
	// (defaults to the offset basis, i.e. start of a fresh digest).
	inline std::uint64_t Fnv1a64(const char* data, std::size_t sizeBytes, std::uint64_t hash = FNV1A64_OFFSET_BASIS)
	{
		for (std::size_t i = 0; i < sizeBytes; ++i)
		{
			hash ^= static_cast<unsigned char>(data[i]);
			hash *= FNV1A64_PRIME;
		}
		return hash;
	}

	// Builds the quoted ETag string from the raw hashed fields.
	// Exposed separately from MakeDesignEtag so tests can pin the digest
	// behavior (determinism, field sensitivity, formatting) without files.
	inline std::string MakeEtag(const std::string& designName, std::int64_t mtimeTicks, std::uint64_t sizeBytes, const std::string& endpointPath)
	{
		auto hash = Fnv1a64(designName.data(), designName.size());
		hash = Fnv1a64(reinterpret_cast<const char*>(&mtimeTicks), sizeof(mtimeTicks), hash);
		hash = Fnv1a64(reinterpret_cast<const char*>(&sizeBytes), sizeof(sizeBytes), hash);
		hash = Fnv1a64(endpointPath.data(), endpointPath.size(), hash);

		static constexpr char HEX_DIGITS[] = "0123456789abcdef";
		std::string tag;
		tag.reserve(18);
		tag += '"';
		for (int shift = 60; shift >= 0; shift -= 4)
		{
			tag += HEX_DIGITS[(hash >> shift) & 0xF];
		}
		tag += '"';
		return tag;
	}

	// Locates the design's archive file by stem-matching designName against the
	// extension set DesignCache documents (DesignCache::DESIGN_EXTENSIONS:
	// zip, tgz, tar.gz, tar, gzip, gz). ".tar.gz" is tested before ".gz"/".tar"
	// so the multi-part extension strips whole ("foo" from "foo.tar.gz"). The
	// extension comparison is case-insensitive; the stem comparison is
	// case-sensitive, mirroring DesignCache's stem lookup. Returns an empty path
	// when no archive matches or the directory cannot be read.
	inline std::filesystem::path FindDesignArchiveFile(const std::string& designsDir, const std::string& designName)
	{
		if (designsDir.empty() || designName.empty())
		{
			return {};
		}

		// string_view (not const char* + strlen): sizes are compile-time, no
		// NUL-termination assumptions on string inputs (Codacy CWE-126).
		static constexpr std::string_view DESIGN_ARCHIVE_EXTENSIONS[] = { ".tar.gz", ".tgz", ".zip", ".tar", ".gzip", ".gz" };

		std::error_code ec;
		std::filesystem::directory_iterator dirIt(designsDir, std::filesystem::directory_options::skip_permission_denied, ec);
		if (ec)
		{
			return {};
		}

		for (const auto& entry : dirIt)
		{
			if (!entry.is_regular_file(ec) || ec)
			{
				ec.clear();
				continue;
			}

			const auto filename = entry.path().filename().string();
			for (const auto extension : DESIGN_ARCHIVE_EXTENSIONS)
			{
				if (filename.size() <= extension.size())
				{
					continue;
				}

				const auto stem = filename.substr(0, filename.size() - extension.size());
				if (stem != designName)
				{
					continue;
				}

				// stem matches; confirm the tail is the extension, case-insensitively
				const auto isExtension = std::equal(extension.begin(), extension.end(),
					filename.begin() + static_cast<std::ptrdiff_t>(stem.size()),
					[](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)); });
				if (isExtension)
				{
					return entry.path();
				}
			}
		}

		return {};
	}

	// Computes the strong ETag for a per-design data endpoint: locates the
	// design's archive file under designsDir and hashes (design name, archive
	// mtime, archive size, endpoint path). Returns the quoted tag, or an empty
	// string when the archive file cannot be found or stat'ed — the endpoint
	// will 404 on load in that case, so no caching headers are emitted.
	inline std::string MakeDesignEtag(const std::string& designsDir, const std::string& designName, const std::string& endpointPath)
	{
		const auto archivePath = FindDesignArchiveFile(designsDir, designName);
		if (archivePath.empty())
		{
			return "";
		}

		std::error_code ec;
		const auto mtime = std::filesystem::last_write_time(archivePath, ec);
		if (ec)
		{
			return "";
		}
		const auto sizeBytes = std::filesystem::file_size(archivePath, ec);
		if (ec)
		{
			return "";
		}

		// file_time_type ticks are stable across server restarts on a given
		// platform, which is all an ETag requires: it is only ever compared
		// against itself, never interpreted as a timestamp.
		const auto mtimeTicks = static_cast<std::int64_t>(mtime.time_since_epoch().count());
		return MakeEtag(designName, mtimeTicks, sizeBytes, endpointPath);
	}

	// RFC 7232 If-None-Match evaluation using weak comparison (the spec mandates
	// weak comparison for If-None-Match even against strong validators like
	// ours): the header may be "*" (matches any current representation) or a
	// comma-separated list of entity-tags, each optionally weak-prefixed
	// (W/"..."); comparison covers the opaque quoted part only.
	inline bool IfNoneMatchMatches(const std::string& header, const std::string& etag)
	{
		if (header.empty() || etag.empty())
		{
			return false;
		}

		constexpr const char* OWS = " \t";
		std::size_t start = 0;
		while (start <= header.size())
		{
			const auto comma = header.find(',', start);
			const auto end = (comma == std::string::npos) ? header.size() : comma;

			auto tokenStart = start;
			auto tokenEnd = end;
			while (tokenStart < tokenEnd && std::strchr(OWS, header[tokenStart]) != nullptr) ++tokenStart;
			while (tokenEnd > tokenStart && std::strchr(OWS, header[tokenEnd - 1]) != nullptr) --tokenEnd;
			auto token = header.substr(tokenStart, tokenEnd - tokenStart);

			if (token.size() >= 2 && (token[0] == 'W' || token[0] == 'w') && token[1] == '/')
			{
				token = token.substr(2);
			}

			if (token == "*" || token == etag)
			{
				return true;
			}

			if (comma == std::string::npos)
			{
				break;
			}
			start = comma + 1;
		}

		return false;
	}
}
