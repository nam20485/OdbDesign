#include <gtest/gtest.h>
#include <filesystem>
#include "FileReader.h"
#include "Fixtures/TestDataFixture.h"

using namespace std::filesystem;
using namespace Odb::Test::Fixtures;
using namespace testing;
using namespace Utils;

namespace Odb::Test
{
	static inline constexpr char FILE_CONTENTS[] = "Hello, World!";
	static inline constexpr char FILE_NAME[] = "filereader_test1.txt";

	TEST_F(TestDataFixture, Test_FileReaderRead_FileExists)
	{		
		auto filePath = getTestDataFilePath(FILE_NAME);
		ASSERT_FALSE(filePath.empty());
		ASSERT_TRUE(exists(filePath));
	}

	TEST_F(TestDataFixture, Test_FileReaderRead_Buffered)
	{		
		auto filePath = getTestDataFilePath(FILE_NAME);
		ASSERT_FALSE(filePath.empty());
		ASSERT_TRUE(exists(filePath));

		FileReader fr(filePath);

		auto fileSize = file_size(filePath);
		auto read = fr.Read(FileReader::BufferStrategy::Buffered);
		ASSERT_EQ(read, fileSize);

		auto& buffer = fr.GetBuffer();
		ASSERT_EQ(buffer.size(), fileSize);
		ASSERT_STREQ(buffer.data(), FILE_CONTENTS);		
	}

	TEST_F(TestDataFixture, Test_FileReaderRead_Unbuffered)
	{
		auto filePath = getTestDataFilePath(FILE_NAME);
		ASSERT_FALSE(filePath.empty());
		ASSERT_TRUE(exists(filePath));

		FileReader fr(filePath);

		auto fileSize = file_size(filePath);
		auto read = fr.Read(FileReader::BufferStrategy::Unbuffered);
		ASSERT_EQ(read, fileSize);

		auto& buffer = fr.GetBuffer();
		ASSERT_EQ(buffer.size(), fileSize);
		ASSERT_STREQ(buffer.data(), FILE_CONTENTS);
	}
}