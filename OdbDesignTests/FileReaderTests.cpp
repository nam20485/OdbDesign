#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include "FileReader.h"
#include "Fixtures//FileArchiveLoadFixture.h"

using namespace std::filesystem;
using namespace Odb::Test::Fixtures;
using namespace testing;
using namespace Utils;

namespace Odb::Test
{
	static inline constexpr char FILE_CONTENTS[] = "Hello, World!";

	TEST_F(FileArchiveLoadFixture, Test_FileReaderRead_Buffered)
	{
		auto fileReaderTestPath = path(getTestDataDir());
		fileReaderTestPath /= "filereader";
		auto filePath = fileReaderTestPath / "filereader_test1.txt";

		ASSERT_TRUE(exists(filePath));

		FileReader fr(filePath);

		auto fileSize = file_size(filePath);
		auto read = fr.Read();
		ASSERT_EQ(read, fileSize);

		auto& buffer = fr.GetBuffer();
		ASSERT_EQ(buffer.size(), fileSize);
		ASSERT_STREQ(buffer.data(), FILE_CONTENTS);		
	}

	TEST_F(FileArchiveLoadFixture, Test_FileReaderRead_Unbuffered)
	{
		auto fileReaderTestPath = path(getTestDataDir());
		fileReaderTestPath /= "filereader";
		auto filePath = fileReaderTestPath / "filereader_test1.txt";

		ASSERT_TRUE(exists(filePath));

		FileReader fr(filePath, true);

		auto fileSize = file_size(filePath);
		auto read = fr.Read();
		ASSERT_EQ(read, fileSize);

		auto& buffer = fr.GetBuffer();
		ASSERT_EQ(buffer.size(), fileSize);
		ASSERT_STREQ(buffer.data(), FILE_CONTENTS);
	}
}