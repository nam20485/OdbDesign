#include <gtest/gtest.h>
#include <filesystem>
#include "Fixtures/FileArchiveLoadFixture.h"
#include "libarchive_extract.h"
#include "ArchiveExtractor.h"

using namespace std::filesystem;
using namespace Odb::Test::Fixtures;
using namespace testing;
using namespace Utils;

namespace Odb::Test
{
	static inline constexpr char FILE_CONTENTS[] = "Hello, World!\n\n";

	TEST_F(FileArchiveLoadFixture, Test_LibArchive_CompressDir)
	{
		std::string fileArchiveOut;
		compress_dir(getTestDataFilesDir().string().c_str(), getTestDataFilesDir().string().c_str(), "files_libarchive", fileArchiveOut);

		ASSERT_TRUE(exists(fileArchiveOut));
	}

	TEST_F(FileArchiveLoadFixture, Test_ArchiveExtractor_CompressDir)
	{
		std::string fileArchiveOut;
		ArchiveExtractor::CompressDir(getTestDataFilesDir().string(), getTestDataFilesDir().string(), "files_archiveextractor", fileArchiveOut);

		ASSERT_TRUE(exists(fileArchiveOut));
	}
}