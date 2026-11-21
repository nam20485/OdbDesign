#include <gtest/gtest.h>
#include <filesystem>
#include <stdexcept>
#include <cstring>
#include <memory>
#include <vector>
#include "FileReader.h"
#include "Fixtures/TestDataFixture.h"
#include "Fixtures/TestUtils.h"

using namespace std::filesystem;
using namespace Odb::Test::Fixtures;
using namespace Odb::Test::Utils;
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

	TEST(FileReaderTests, Test_FileReaderRead_MissingFile_ReturnsZeroAndEmptyBuffer)
	{
		auto missingPath = temp_directory_path() / "odbtest_does_not_exist_9f2c1.bin";

		FileReader frBuffered(missingPath);
		EXPECT_EQ(frBuffered.Read(FileReader::BufferStrategy::Buffered), 0LL);
		EXPECT_TRUE(frBuffered.GetBuffer().empty());

		FileReader frUnbuffered(missingPath);
		EXPECT_EQ(frUnbuffered.Read(FileReader::BufferStrategy::Unbuffered), 0LL);
		EXPECT_TRUE(frUnbuffered.GetBuffer().empty());
	}

	TEST(FileReaderTests, Test_FileReaderRead_EmptyFile_ReturnsZeroAndEmptyBuffer)
	{
		auto emptyFile = TestUtils::createManagedTempFile("", ".empty");

		FileReader frBuffered(emptyFile->path());
		EXPECT_EQ(frBuffered.Read(FileReader::BufferStrategy::Buffered), 0LL);
		EXPECT_TRUE(frBuffered.GetBuffer().empty());

		FileReader frUnbuffered(emptyFile->path());
		EXPECT_EQ(frUnbuffered.Read(FileReader::BufferStrategy::Unbuffered), 0LL);
		EXPECT_TRUE(frUnbuffered.GetBuffer().empty());
	}

	TEST(FileReaderTests, Test_FileReaderRead_BinaryContent_NulAndHighBitBytes_Buffered)
	{
		// NUL and high-bit bytes: Read() must preserve the EXACT bytes,
		// unlike the existing text-based tests that use ASSERT_STREQ
		const std::vector<char> binary{ '\x41', '\x00', '\x42', '\xE9', '\xFF', '\x00' };
		auto binaryFile = std::make_unique<TestUtils::TempResource>(
			TestUtils::createTempBinaryFile(
				std::vector<uint8_t>(binary.begin(), binary.end()), ".bin"));

		FileReader fr(binaryFile->path());
		auto read = fr.Read(FileReader::BufferStrategy::Buffered);
		ASSERT_EQ(read, static_cast<long long>(binary.size()));

		const auto& buffer = fr.GetBuffer();
		ASSERT_EQ(buffer.size(), binary.size());
		EXPECT_TRUE(std::equal(binary.begin(), binary.end(), buffer.begin()));
	}

	TEST(FileReaderTests, Test_FileReaderRead_BinaryContent_NulAndHighBitBytes_Unbuffered)
	{
		const std::vector<char> binary{ '\x00', '\x7F', '\x80', '\xC3', '\x41' };
		auto binaryFile = std::make_unique<TestUtils::TempResource>(
			TestUtils::createTempBinaryFile(
				std::vector<uint8_t>(binary.begin(), binary.end()), ".bin"));

		FileReader fr(binaryFile->path());
		auto read = fr.Read(FileReader::BufferStrategy::Unbuffered);
		ASSERT_EQ(read, static_cast<long long>(binary.size()));

		const auto& buffer = fr.GetBuffer();
		ASSERT_EQ(buffer.size(), binary.size());
		EXPECT_TRUE(std::equal(binary.begin(), binary.end(), buffer.begin()));
	}

	// documented actual behavior: repeating Read() on the SAME unchanged file
	// without Clear() still yields the correct bytes — the buffered strategy
	// appends the second read and then resize(read) truncates back to the
	// first copy, so size and the bytes within size stay correct. NOTE: the
	// buffer is not NUL-terminated (bytes beyond size() are stale leftovers
	// from the appended second read), so only the exact range is compared;
	// the grown-file hazard is covered by the changed-file tripwire below
	TEST(FileReaderTests, Test_FileReaderRead_RepeatWithoutClear_SameFile_BufferCorrect)
	{
		auto tempFile = TestUtils::createManagedTempFile(FILE_CONTENTS, ".txt");

		FileReader fr(tempFile->path());
		ASSERT_EQ(fr.Read(), static_cast<long long>(strlen(FILE_CONTENTS)));

		auto second = fr.Read();
		EXPECT_EQ(second, static_cast<long long>(strlen(FILE_CONTENTS)));
		ASSERT_EQ(fr.GetBuffer().size(), strlen(FILE_CONTENTS));
		EXPECT_TRUE(std::equal(FILE_CONTENTS, FILE_CONTENTS + strlen(FILE_CONTENTS),
							   fr.GetBuffer().begin()));
	}

	// KNOWN DEFECT TRIPWIRE (currently FAILS): the buffered strategy appends
	// into the existing buffer without clearing it first
	// (Utils/FileReader.cpp:57 m_buffer.insert), then resize()s to the fresh
	// read count (:62). Re-reading a file that GREW between reads therefore
	// yields a MIX of old bytes followed by the first bytes of the new
	// content, reported with the new size. This test asserts the CORRECT
	// behavior (the buffer holds the current file content); fixing the
	// implementation should make it pass.
	TEST(FileReaderTests, Test_FileReaderRead_RepeatAfterFileGrew_BufferHoldsCurrentContent)
	{
		const std::string originalContents = "AAAA";
		const std::string grownContents = "BBBBBBBBBBBB";

		auto tempFile = TestUtils::createManagedTempFile(originalContents, ".grow");

		FileReader fr(tempFile->path());
		ASSERT_EQ(fr.Read(), static_cast<long long>(originalContents.size()));

		// grow the file between the two reads
		{
			std::ofstream grow(tempFile->path(), std::ios::binary | std::ios::trunc);
			grow.write(grownContents.c_str(), static_cast<std::streamsize>(grownContents.size()));
			ASSERT_TRUE(grow.good());
		}

		auto second = fr.Read();
		EXPECT_EQ(second, static_cast<long long>(grownContents.size()));
		ASSERT_EQ(fr.GetBuffer().size(), grownContents.size());
		EXPECT_EQ(std::string(fr.GetBuffer().data(), fr.GetBuffer().size()), grownContents);
	}

	TEST(FileReaderTests, Test_FileReaderClear_ResetAllowsCleanReread)
	{
		auto tempFile = TestUtils::createManagedTempFile(FILE_CONTENTS, ".txt");

		FileReader fr(tempFile->path());
		ASSERT_EQ(fr.Read(), static_cast<long long>(strlen(FILE_CONTENTS)));

		fr.Clear();
		EXPECT_TRUE(fr.GetBuffer().empty());

		EXPECT_EQ(fr.Read(), static_cast<long long>(strlen(FILE_CONTENTS)));
		ASSERT_EQ(fr.GetBuffer().size(), strlen(FILE_CONTENTS));
		EXPECT_STREQ(fr.GetBuffer().data(), FILE_CONTENTS);
	}

	TEST(FileReaderTests, Test_FileReaderRead_Directory_BufferedReturnsZero)
	{
		auto tempDir = TestUtils::createManagedTempDirectory("filereader");

		// buffered: the stream opens but reads nothing from a directory
		FileReader frBuffered(tempDir->path());
		EXPECT_EQ(frBuffered.Read(FileReader::BufferStrategy::Buffered), 0LL);
		EXPECT_TRUE(frBuffered.GetBuffer().empty());
	}

	TEST(FileReaderTests, Test_FileReaderRead_Directory_Unbuffered_ThrowsFilesystemError)
	{
		auto tempDir = TestUtils::createManagedTempDirectory("filereader");

		// unbuffered: file_size() on a directory throws std::filesystem_error
		FileReader fr(tempDir->path());
		EXPECT_THROW(fr.Read(FileReader::BufferStrategy::Unbuffered),
					 std::filesystem::filesystem_error);
	}
}