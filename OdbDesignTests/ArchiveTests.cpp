#include <gtest/gtest.h>
#include <filesystem>
#include "Fixtures/TestDataFixture.h"
#include "Fixtures/TestUtils.h"
#include "libarchive_extract.h"
#include "ArchiveExtractor.h"
#include <string>

using namespace std::filesystem;
using namespace Odb::Test::Fixtures;
using namespace Odb::Test::Utils;
using namespace testing;
using namespace Utils;

namespace Odb::Test
{
	TEST_F(TestDataFixture, Test_LibArchive_CompressDir)
	{
		// Compress into a per-test managed temp dir rather than the shared system temp
		// root. compress_dir() derives the output filename entirely from archiveName
		// (no PID/unique component), so a fixed name in the shared temp root would be
		// clobbered/raced by a concurrent invocation of this same test under ctest -j.
		// The managed dir both uniquifies the path and removes the artifact on scope exit.
		auto destDir = TestUtils::createManagedTempDirectory("odbtest_compress_libarchive");
		std::string fileArchiveOut;
		compress_dir(getTestDataFilesDir().string().c_str(), destDir->path().string().c_str(), "files_libarchive", fileArchiveOut);

		ASSERT_TRUE(exists(fileArchiveOut));
	}
}