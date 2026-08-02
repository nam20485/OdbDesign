#include <gtest/gtest.h>
#include <filesystem>
#include "Fixtures/TestDataFixture.h"
#include "libarchive_extract.h"
#include "ArchiveExtractor.h"
#include <string>

using namespace std::filesystem;
using namespace Odb::Test::Fixtures;
using namespace testing;
using namespace Utils;

namespace Odb::Test
{
	TEST_F(TestDataFixture, Test_LibArchive_CompressDir)
	{
		std::string fileArchiveOut;
		// Write the compressed output to the system temp dir, not the shared test-data
		// directory, so parallel runs don't leave artifacts in (or race on) TEST_DATA.
		compress_dir(getTestDataFilesDir().string().c_str(), temp_directory_path().string().c_str(), "files_libarchive", fileArchiveOut);

		ASSERT_TRUE(exists(fileArchiveOut));
	}	
}