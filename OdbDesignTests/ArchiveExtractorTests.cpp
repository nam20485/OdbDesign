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
	static inline constexpr char FILE_CONTENTS[] = "Hello, World!";

	TEST_F(TestDataFixture, Test_ArchiveExtractor_CompressDir)
	{
		std::string fileArchiveOut;
		ArchiveExtractor::CompressDir(getTestDataFilesDir().string(), getTestDataDir().string(), "files_archiveextractor", fileArchiveOut);

		ASSERT_STRNE(fileArchiveOut.c_str(), "");
		ASSERT_TRUE(exists(fileArchiveOut));
	}
}