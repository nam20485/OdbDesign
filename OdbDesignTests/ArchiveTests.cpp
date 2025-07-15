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
	static inline constexpr char FILE_CONTENTS[] = "Hello, World!";

	TEST_F(TestDataFixture, Test_LibArchive_CompressDir)
	{
		std::string fileArchiveOut;
		compress_dir(getTestDataFilesDir().string().c_str(), getTestDataDir().string().c_str(), "files_libarchive", fileArchiveOut);

		ASSERT_TRUE(exists(fileArchiveOut));
	}	
}