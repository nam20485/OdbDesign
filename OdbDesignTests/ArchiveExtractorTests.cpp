#include <gtest/gtest.h>
#include "Fixtures/TestDataFixture.h"
#include "Fixtures/TestUtils.h"
#include "ArchiveExtractor.h"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

//using namespace Odb::Lib::App;
//using namespace Odb::Lib::FileModel;
using namespace Odb::Test::Fixtures;
using namespace Odb::Test::Utils;
using namespace std::filesystem;
using namespace Utils;

namespace Odb::Test
{
	TEST_F(TestDataFixture, Test_ArchiveExtractor_CompressDir)
	{
		// Compress into a per-test managed temp dir rather than the shared system temp
		// root. compress_dir() derives the output filename entirely from archiveName
		// (no PID/unique component), so a fixed name in the shared temp root would be
		// clobbered/raced by a concurrent invocation of this same test under ctest -j.
		// The managed dir both uniquifies the path and removes the artifact on scope exit.
		auto destDir = TestUtils::createManagedTempDirectory("odbtest_compress_extractor");
		std::string fileArchiveOut;
		ArchiveExtractor::CompressDir(getTestDataFilesDir().string(), destDir->path().string(), "files_archiveextractor", fileArchiveOut);

		ASSERT_STRNE(fileArchiveOut.c_str(), "");
		ASSERT_TRUE(exists(fileArchiveOut));
	}

	TEST_F(TestDataFixture, Test_ArchiveExtractor_DeepTree_LongPaths_RoundTrip)
	{
		// Regression (Windows, security-audit F003 follow-up): extraction
		// destinations nest the archive stem twice (dest/<stem>/<stem>/... -
		// findRootDir descends one level), and CreateDirectoryW caps
		// directory paths at 248 characters unless the \\?\ long-path form is
		// used. libarchive's own write path applied that prefix silently
		// (__la_win_permissive_name); the hardened ofstream/std::filesystem
		// write path must apply it to the destination root instead. Build a
		// tree deep enough to cross the cap even under a short temp root and
		// require the full compress -> extract -> content round-trip to
		// succeed (real behavior, not a string check).
		auto workDir = TestUtils::createManagedTempDirectory("odbtest_deep_tree_roundtrip");
		// entry paths must stay inside ustar limits (155-char prefix +
		// 100-char name for compress_dir), while the EXTRACTED absolute path
		// (temp root + stem + internal root + entry) crosses 260 characters.
		const std::string treeName = "deep_tree_root_name_padding_padding_padding_0123456789";
		const std::string segmentName = "segment_padding_0123456789_";
		auto srcDir = workDir->path() / treeName;

		std::filesystem::path leafDir = srcDir;
		for (int i = 0; i < 3; ++i)
		{
			leafDir /= segmentName + std::to_string(i);
		}
		std::filesystem::create_directories(leafDir);
		const std::string payload = "deep-leaf-payload";
		{
			std::ofstream out(leafDir / "leaf.bin", std::ios::binary);
			out << payload;
		}

		std::string archiveOut;
		ASSERT_TRUE(ArchiveExtractor::CompressDir(srcDir.string(), workDir->path().string(), treeName, archiveOut));
		ASSERT_STRNE(archiveOut.c_str(), "");
		ASSERT_TRUE(exists(archiveOut));

		ArchiveExtractor extractor(archiveOut);
		ASSERT_TRUE(extractor.Extract());

		// extraction nests the internal root dir under dest/<stem>; mirror
		// the production double-nesting when asserting on the deep leaf
		auto extractedLeaf = path(extractor.GetExtractionDirectory()) / treeName;
		for (int i = 0; i < 3; ++i)
		{
			extractedLeaf /= segmentName + std::to_string(i);
		}
		extractedLeaf /= "leaf.bin";
		ASSERT_TRUE(exists(extractedLeaf)) << "deepest path: " << extractedLeaf.string();

		std::ifstream in(extractedLeaf, std::ios::binary);
		std::string readBack((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
		EXPECT_EQ(readBack, payload);
	}
}