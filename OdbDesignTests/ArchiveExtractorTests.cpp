#include <gtest/gtest.h>
#include "Fixtures/TestDataFixture.h"
#include "Fixtures/TestUtils.h"
#include "ArchiveExtractor.h"
#include <filesystem>
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
}