#include <gtest/gtest.h>
#include "Fixtures/TestDataFixture.h"
#include "ArchiveExtractor.h"
#include <filesystem>
#include <string>

//using namespace Odb::Lib::App;
//using namespace Odb::Lib::FileModel;
using namespace Odb::Test::Fixtures;
using namespace std::filesystem;
using namespace Utils;

namespace Odb::Test
{
	TEST_F(TestDataFixture, Test_ArchiveExtractor_CompressDir)
	{
		std::string fileArchiveOut;
		// Write the compressed output to the system temp dir, not the shared test-data
		// directory, so parallel runs don't leave artifacts in (or race on) TEST_DATA.
		ArchiveExtractor::CompressDir(getTestDataFilesDir().string(), temp_directory_path().string(), "files_archiveextractor", fileArchiveOut);

		ASSERT_STRNE(fileArchiveOut.c_str(), "");
		ASSERT_TRUE(exists(fileArchiveOut));
	}
}