#include <gtest/gtest.h>
#include "Fixtures/TestUtils.h"
#include "ArchiveExtractor.h"
#include "PathSafety.h"
#include "ProcessSpawn.h"
#include "fastmove.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

using namespace Odb::Test::Utils;
using namespace Utils;
using namespace std::filesystem;

namespace Odb::Test
{
	namespace
	{
		// ---- minimal hand-rolled ustar writer ---------------------------------
		// Security-audit F003 tests need archives with HOSTILE entry names
		// (absolute paths, ".." traversal, restored modes). compress_dir cannot
		// produce those, so tests write raw ustar headers byte-for-byte and let
		// the extractor prove it refuses them.

		void write_octal(char* field, std::size_t fieldSize, std::uint64_t value)
		{
			// zero-terminated octal ASCII in a fixed-width field
			std::snprintf(field, fieldSize, "%0*llo", static_cast<int>(fieldSize - 1), value);
		}

		void write_ustar_header(std::ostream& os, const std::string& pathname, std::uintmax_t size, char typeflag, unsigned mode = 0644, const std::string& linkname = "")
		{
			char header[512]{};
			std::memcpy(header, pathname.c_str(), std::min<std::size_t>(pathname.size(), 100));
			write_octal(header + 100, 8, mode);
			write_octal(header + 108, 8, 0);			// uid
			write_octal(header + 116, 8, 0);			// gid
			write_octal(header + 124, 12, size);
			write_octal(header + 136, 12, 0);			// mtime
			std::memset(header + 148, ' ', 8);			// chksum field = spaces while summing
			header[156] = typeflag;
			if (!linkname.empty())
			{
				std::memcpy(header + 157, linkname.c_str(), std::min<std::size_t>(linkname.size(), 100));
			}
			std::memcpy(header + 257, "ustar", 5);		// magic "ustar\0" at [257..262]
			header[262] = '\0';
			header[263] = '0';							// POSIX version "00" at [263..264]
			header[264] = '0';

			unsigned checksum = 0;
			for (const auto byte : header)
			{
				checksum += static_cast<unsigned char>(byte);
			}
			// 6 octal digits + NUL + space
			std::snprintf(header + 148, 8, "%06o", checksum);
			header[154] = '\0';
			header[155] = ' ';

			os.write(header, sizeof(header));
		}

		void write_ustar_entry(std::ostream& os, const std::string& pathname, const std::string& content, unsigned mode = 0644)
		{
			write_ustar_header(os, pathname, content.size(), '0', mode);
			os.write(content.data(), content.size());
			const auto padding = (512 - (content.size() % 512)) % 512;
			static const char zeros[512]{};
			os.write(zeros, padding);
		}

		void write_ustar_hardlink_entry(std::ostream& os, const std::string& pathname, const std::string& target)
		{
			// a hardlink is a zero-size header-only entry with the target in
			// the linkname field
			write_ustar_header(os, pathname, 0, '1', 0644, target);
		}

		void write_ustar_end(std::ostream& os)
		{
			static const char zeros[1024]{};
			os.write(zeros, sizeof(zeros));
		}

		path write_hostile_tar(const path& archivePath, const std::string& entryName, const std::string& content, unsigned mode = 0644)
		{
			std::ofstream os(archivePath, std::ios::binary | std::ios::trunc);
			write_ustar_entry(os, entryName, content, mode);
			write_ustar_end(os);
			return archivePath;
		}

		std::string read_file(const path& p)
		{
			std::ifstream ifs(p, std::ios::binary);
			return std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
		}
	}

	// =========================================================================
	// F002: upload filename validation vocabulary
	// =========================================================================

	TEST(ArchiveSecurity, PathSafety_AcceptsLegalFilenameComponents)
	{
		EXPECT_TRUE(is_safe_filename_component("design.tgz"));
		EXPECT_TRUE(is_safe_filename_component("my design (rev2).tar.gz"));
		EXPECT_TRUE(is_safe_filename_component("a"));
		EXPECT_TRUE(is_safe_filename_component("hidden.tgz"));
	}

	TEST(ArchiveSecurity, PathSafety_RejectsHostileFilenameComponents)
	{
		// traversal / separators
		EXPECT_FALSE(is_safe_filename_component(""));
		EXPECT_FALSE(is_safe_filename_component("a/b"));
		EXPECT_FALSE(is_safe_filename_component("a\\b"));
		EXPECT_FALSE(is_safe_filename_component(".."));
		EXPECT_FALSE(is_safe_filename_component("."));
		EXPECT_FALSE(is_safe_filename_component("..."));
		EXPECT_FALSE(is_safe_filename_component("../escape"));
		// rooted / drive / NTFS stream forms
		EXPECT_FALSE(is_safe_filename_component("/etc/passwd"));
		EXPECT_FALSE(is_safe_filename_component("C:x"));
		EXPECT_FALSE(is_safe_filename_component("design.tgz:stream"));
		// control characters (newline, NUL byte, DEL)
		EXPECT_FALSE(is_safe_filename_component("bad\nname"));
		EXPECT_FALSE(is_safe_filename_component(std::string("bad\0name", 8)));
		EXPECT_FALSE(is_safe_filename_component("bad\x7Fname"));
		// Windows name normalization aliases
		EXPECT_FALSE(is_safe_filename_component("design."));
		EXPECT_FALSE(is_safe_filename_component("design "));
		// over-length
		EXPECT_FALSE(is_safe_filename_component(std::string(256, 'a')));
	}

	TEST(ArchiveSecurity, PathSafety_RelativeEntryPathRules)
	{
		// interior structure is fine - nested ODB++ members use it
		EXPECT_TRUE(is_safe_relative_entry_path("step/layers/features.Z"));
		EXPECT_TRUE(is_safe_relative_entry_path("matrix/matrix"));
		EXPECT_FALSE(is_safe_relative_entry_path(""));
		// rooted and traversal forms escape the base when joined
		EXPECT_FALSE(is_safe_relative_entry_path("/etc/pwn"));
		EXPECT_FALSE(is_safe_relative_entry_path("\\etc\\pwn"));
		EXPECT_FALSE(is_safe_relative_entry_path("../outside"));
		EXPECT_FALSE(is_safe_relative_entry_path("step/../../outside"));
		EXPECT_FALSE(is_safe_relative_entry_path("step\\..\\outside"));
		// empty interior components are malformed; a single trailing
		// separator is legitimate directory-entry spelling and tolerated
		EXPECT_FALSE(is_safe_relative_entry_path("step//features"));
		EXPECT_TRUE(is_safe_relative_entry_path("step/layers/"));
		EXPECT_FALSE(is_safe_relative_entry_path("bad\nname"));
	}

	// =========================================================================
	// F002: fastmove no-overwrite contract
	// =========================================================================

	TEST(ArchiveSecurity, FastMove_NoOverwrite_RefusesExistingDestination)
	{
		auto dir = TestUtils::createManagedTempDirectory("odbtest_fastmove");
		const auto src = dir->path() / "src.tmp";
		const auto dst = dir->path() / "design.tgz";
		{
			std::ofstream(src, std::ios::binary | std::ios::trunc) << "new content";
			std::ofstream(dst, std::ios::binary | std::ios::trunc) << "existing content";
		}

		std::error_code ec;
		fastmove_file(src, dst, /*overwriteExisting=*/false, ec);

		EXPECT_EQ(ec, std::errc::file_exists);
		EXPECT_EQ(read_file(dst), "existing content") << "destination must be untouched";
		EXPECT_TRUE(exists(src)) << "source must survive a refused move";
	}

	TEST(ArchiveSecurity, FastMove_Overwrite_ReplacesDestination)
	{
		auto dir = TestUtils::createManagedTempDirectory("odbtest_fastmove_ow");
		const auto src = dir->path() / "src.tmp";
		const auto dst = dir->path() / "design.tgz";
		{
			std::ofstream ofs(dst, std::ios::binary | std::ios::trunc);
			ofs << "old";
		}
		{
			std::ofstream ofs(src, std::ios::binary | std::ios::trunc);
			ofs << "new";
		}

		std::error_code ec;
		fastmove_file(src, dst, /*overwriteExisting=*/true, ec);

		EXPECT_EQ(ec.value(), 0);
		EXPECT_EQ(read_file(dst), "new");
		EXPECT_FALSE(exists(src));
	}

	TEST(ArchiveSecurity, FastMove_NoOverwrite_AllowsFreshDestination)
	{
		auto dir = TestUtils::createManagedTempDirectory("odbtest_fastmove_fresh");
		const auto src = dir->path() / "src.tmp";
		const auto dst = dir->path() / "design.tgz";
		{
			std::ofstream ofs(src, std::ios::binary | std::ios::trunc);
			ofs << "fresh";
		}

		std::error_code ec;
		fastmove_file(src, dst, /*overwriteExisting=*/false, ec);

		EXPECT_EQ(ec.value(), 0);
		EXPECT_EQ(read_file(dst), "fresh");
	}

	// =========================================================================
	// F003: hostile archive entries must not escape the extraction root
	// =========================================================================

	TEST(ArchiveSecurity, Extract_AbsoluteEntryName_Refused)
	{
		auto dir = TestUtils::createManagedTempDirectory("odbtest_f003_abs");
		const auto archive = write_hostile_tar(dir->path() / "hostile.tar", "/pwn-f003-absolute", "payload");
		const auto dest = dir->path() / "dest";
		create_directories(dest);

		ArchiveExtractor extractor(archive.string());
		EXPECT_FALSE(extractor.Extract(dest.string())) << "absolute entry name must fail extraction";
		EXPECT_TRUE(is_empty(dest)) << "nothing may be written for a rejected entry";
	}

	TEST(ArchiveSecurity, Extract_TraversalEntryName_Refused)
	{
		auto dir = TestUtils::createManagedTempDirectory("odbtest_f003_trav");
		const auto archive = write_hostile_tar(dir->path() / "hostile.tar", "../outside-f003", "payload");
		const auto dest = dir->path() / "dest";
		create_directories(dest);

		ArchiveExtractor extractor(archive.string());
		EXPECT_FALSE(extractor.Extract(dest.string())) << "traversal entry name must fail extraction";
		EXPECT_TRUE(is_empty(dest));
		EXPECT_FALSE(exists(dir->path() / "outside-f003")) << "nothing may land outside the extraction root";
	}

	TEST(ArchiveSecurity, Extract_BackslashTraversalEntryName_Refused)
	{
		auto dir = TestUtils::createManagedTempDirectory("odbtest_f003_bslash");
		const auto archive = write_hostile_tar(dir->path() / "hostile.tar", "..\\outside-f003", "payload");
		const auto dest = dir->path() / "dest";
		create_directories(dest);

		ArchiveExtractor extractor(archive.string());
		EXPECT_FALSE(extractor.Extract(dest.string()));
		EXPECT_FALSE(exists(dir->path() / "outside-f003"));
	}

	TEST(ArchiveSecurity, Extract_ControlCharacterEntryName_Refused)
	{
		auto dir = TestUtils::createManagedTempDirectory("odbtest_f003_ctrl");
		const auto archive = write_hostile_tar(dir->path() / "hostile.tar", "bad\nname", "payload");
		const auto dest = dir->path() / "dest";
		create_directories(dest);

		ArchiveExtractor extractor(archive.string());
		EXPECT_FALSE(extractor.Extract(dest.string()));
	}

	TEST(ArchiveSecurity, Extract_AttackerModesNotRestored)
	{
		auto dir = TestUtils::createManagedTempDirectory("odbtest_f003_mode");
		// entry declares mode 0777 - extractor must NOT restore it
		const auto archive = write_hostile_tar(dir->path() / "hostile.tar", "shady", "payload", 0777);
		const auto dest = dir->path() / "dest";
		create_directories(dest);

		ArchiveExtractor extractor(archive.string());
		EXPECT_TRUE(extractor.Extract(dest.string())) << "a legal entry must still extract";

		const auto extracted = dest / "shady";
		ASSERT_TRUE(exists(extracted));
#if !defined(_WIN32)
		const auto execBits = perms::owner_exec | perms::group_exec | perms::others_exec;
		EXPECT_EQ(status(extracted).permissions() & execBits, perms::none)
			<< "execute bits from the archive entry must not survive extraction";
#endif
		EXPECT_EQ(read_file(extracted), "payload");
	}

	TEST(ArchiveSecurity, Extract_LegalRelativeEntries_RoundTrip)
	{
		// over-rejection guard: the ODB++ layout (nested relative paths) must
		// keep extracting normally
		auto dir = TestUtils::createManagedTempDirectory("odbtest_f003_good");
		const auto archive = dir->path() / "good.tar";
		{
			std::ofstream os(archive, std::ios::binary | std::ios::trunc);
			write_ustar_entry(os, "step/layers/top/features", "feature content");
			write_ustar_entry(os, "matrix/matrix", "matrix content");
			write_ustar_end(os);
		}
		const auto dest = dir->path() / "dest";
		create_directories(dest);

		ArchiveExtractor extractor(archive.string());
		EXPECT_TRUE(extractor.Extract(dest.string()));
		EXPECT_EQ(read_file(dest / "step/layers/top/features"), "feature content");
		EXPECT_EQ(read_file(dest / "matrix/matrix"), "matrix content");
	}


	// =========================================================================
	// kilo round-1 follow-ups: Windows-alias components, reserved device
	// names, hardlink entries, pre-planted symlinks inside destDir
	// =========================================================================

	TEST(ArchiveSecurity, PathSafety_WindowsAliasComponents_Rejected)
	{
		// ".. " (dot-dot-space) is ".." after Windows name normalization - a
		// traversal that passes byte-equality checks
		EXPECT_FALSE(is_safe_relative_entry_path(".. "));
		EXPECT_FALSE(is_safe_relative_entry_path("x/.. /y"));
		EXPECT_FALSE(is_safe_relative_entry_path("step/file."));
		EXPECT_FALSE(is_safe_relative_entry_path("step/file "));
		EXPECT_FALSE(is_safe_filename_component(".. "));
	}

	TEST(ArchiveSecurity, PathSafety_WindowsReservedDeviceNames_Rejected)
	{
		EXPECT_FALSE(is_safe_filename_component("CON"));
		EXPECT_FALSE(is_safe_filename_component("NUL.tgz"));
		EXPECT_FALSE(is_safe_filename_component("com1"));
		EXPECT_FALSE(is_safe_relative_entry_path("step/LPT9"));
		EXPECT_FALSE(is_safe_relative_entry_path("aux/data"));
		// similar-but-not-reserved names stay legal
		EXPECT_TRUE(is_safe_filename_component("console.tgz"));
		EXPECT_TRUE(is_safe_filename_component("design.tgz"));
		EXPECT_TRUE(is_safe_relative_entry_path("step/contrast"));
	}

	TEST(ArchiveSecurity, Extract_HardlinkEntry_Refused)
	{
		// tar hardlinks surface as AE_IFREG with the target in the link
		// field - they must not extract as empty/truncating files
		auto dir = TestUtils::createManagedTempDirectory("odbtest_kilo_hardlink");
		const auto archive = dir->path() / "hostile.tar";
		{
			std::ofstream os(archive, std::ios::binary | std::ios::trunc);
			write_ustar_entry(os, "real", "legit content");
			write_ustar_hardlink_entry(os, "evil", "real");
			write_ustar_end(os);
		}
		const auto dest = dir->path() / "dest";
		create_directories(dest);

		ArchiveExtractor extractor(archive.string());
		EXPECT_FALSE(extractor.Extract(dest.string())) << "hardlink entry must fail extraction";
	}

	TEST(ArchiveSecurity, Extract_PrePlantedSymlinkInsideDest_Refused)
	{
		// a symlink planted under destDir must not redirect extraction
		auto dir = TestUtils::createManagedTempDirectory("odbtest_kilo_symlink");
		const auto archive = dir->path() / "evil.tar";
		{
			std::ofstream os(archive, std::ios::binary | std::ios::trunc);
			write_ustar_entry(os, "step/hole/features", "payload");
			write_ustar_end(os);
		}
		const auto dest = dir->path() / "dest";
		const auto outside = dir->path() / "outside";
		create_directories(outside);
		create_directories(dest / "step");
#if !defined(_WIN32)
		create_directory_symlink(outside, dest / "step" / "hole");
#endif

		ArchiveExtractor extractor(archive.string());
#if !defined(_WIN32)
		EXPECT_FALSE(extractor.Extract(dest.string())) << "extraction through a symlink must fail";
		EXPECT_FALSE(exists(outside / "features")) << "nothing may be written through the symlink";
#else
		GTEST_SKIP() << "symlink planting not testable on Windows runners";
#endif
	}

	// =========================================================================
	// F001: argv-based process spawning - metacharacters are data, not syntax
	// =========================================================================

#if !defined(_WIN32)
	TEST(ArchiveSecurity, RunProcess_MetacharacterArgument_IsDataNotSyntax)
	{
		auto dir = TestUtils::createManagedTempDirectory("odbtest_f001_argv");
		// Under a shell, "touch <dir>/a;touch_shell_pwned" would create
		// <dir>/a and then run the command "touch_shell_pwned". With argv
		// semantics touch receives ONE literal filename and creates exactly
		// that file - nothing else.
		const auto weirdName = std::string("a;touch_shell_pwned");
		const auto exitCode = run_process("touch", { (dir->path() / weirdName).string() });

		ASSERT_EQ(exitCode, 0);
		EXPECT_TRUE(exists(dir->path() / weirdName)) << "the metacharacter name must be taken literally";
		EXPECT_FALSE(exists(dir->path() / "a")) << "a shell would have split the argument at ';'";
	}
#endif

	TEST(ArchiveSecurity, RunProcess_MissingProgram_ReturnsFailure)
	{
		EXPECT_EQ(run_process("odb-nonexistent-tool-f001", { "--probe" }), -1);
	}

	TEST(ArchiveSecurity, GetUncompressedFilePath_HostileName_ReturnsEmpty)
	{
		auto dir = TestUtils::createManagedTempDirectory("odbtest_f001_nested");
		EXPECT_TRUE(ArchiveExtractor::getUncompressedFilePath(dir->path(), "../escape").empty());
		EXPECT_TRUE(ArchiveExtractor::getUncompressedFilePath(dir->path(), "/abs/escape").empty());
		EXPECT_TRUE(ArchiveExtractor::getUncompressedFilePath(dir->path(), "").empty());
	}

	TEST(ArchiveSecurity, GetUncompressedFilePath_LegalNames_StillResolve)
	{
		auto dir = TestUtils::createManagedTempDirectory("odbtest_f001_nested_ok");
		create_directories(dir->path() / "sub");
		{
			std::ofstream ofs(dir->path() / "features", std::ios::binary | std::ios::trunc);
			ofs << "plain";
			std::ofstream ofs2(dir->path() / "sub" / "nested", std::ios::binary | std::ios::trunc);
			ofs2 << "nested";
		}

		EXPECT_EQ(ArchiveExtractor::getUncompressedFilePath(dir->path(), "features"), dir->path() / "features");
		EXPECT_EQ(ArchiveExtractor::getUncompressedFilePath(dir->path(), "sub/nested"), dir->path() / "sub" / "nested");
	}

	TEST(ArchiveSecurity, ExtractLzma_MetacharacterArchivePath_NoShellSideEffects)
	{
		// End-to-end F001: a .lzma archive whose own path contains shell
		// metacharacters (the shape a hostile nested step/layer name
		// produces). The archive is created with xz (7-Zip 25.01 can decode
		// but not create .lzma), then extracted through the argv-spawned 7z.
		// No shell side-effect file may appear at any point.
		if (run_process("7z", { "i" }) != 0)
		{
			GTEST_SKIP() << "7z not available on PATH";
		}
		if (run_process("xz", { "--version" }) != 0)
		{
			GTEST_SKIP() << "xz not available on PATH";
		}

		auto dir = TestUtils::createManagedTempDirectory("odbtest_f001_e2e");
		const auto contentPath = dir->path() / "content.txt";
		{
			std::ofstream ofs(contentPath, std::ios::binary | std::ios::trunc);
			ofs << "lzma payload";
		}

		// create content.txt.lzma beside it, then move it to a metacharacter
		// path - if any shell interpreted this name downstream, "touch" would
		// run as a command instead of being part of the literal filename
		ASSERT_EQ(run_process("xz", { "--format=lzma", "-k", "-f", contentPath.string() }), 0);
		const auto plainArchive = dir->path() / "content.txt.lzma";
		ASSERT_TRUE(exists(plainArchive));
		const auto weirdArchive = dir->path() / "inj;touch pwned_f001.lzma";
		std::error_code renameEc;
		rename(plainArchive, weirdArchive, renameEc);
		ASSERT_EQ(renameEc.value(), 0);

		// 7z writes a lone .lzma stream out under the ARCHIVE STEM: the
		// extracted file is literally named "inj;touch pwned_f001"
		const auto dest = dir->path() / "dest";
		create_directories(dest);

		ArchiveExtractor extractor(weirdArchive.string());
		EXPECT_TRUE(extractor.Extract(dest.string())) << "argv-spawned 7z must extract the metacharacter path";
		EXPECT_EQ(read_file(dest / "inj;touch pwned_f001"), "lzma payload");
		EXPECT_FALSE(exists(dir->path() / "pwned_f001")) << "no shell side effects from the archive path";
		EXPECT_FALSE(exists(current_path() / "pwned_f001")) << "no shell side effects in the process cwd either";
	}
}
