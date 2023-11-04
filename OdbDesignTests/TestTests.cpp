#include <gtest/gtest.h>
#include "FileArchiveLoadFixture.h"
#include <filesystem>

using namespace std::filesystem;

namespace Odb::Test
{
	// Demonstrate some basic assertions.
	TEST(TestTest, BasicAssertions) {
		// Expect two strings not to be equal.
		EXPECT_STRNE("hello", "world");
		// Expect equality.
		EXPECT_EQ(7 * 6, 42);
	}

	TEST_F(FileArchiveLoadFixture, TestDataDirEnvironmentVariablesExists)
	{
		ASSERT_FALSE(getTestDataDir().empty());
	}

	TEST_F(FileArchiveLoadFixture, TestDataDirDirectoryExists)
	{
		ASSERT_FALSE(getTestDataDir().empty());
		EXPECT_TRUE(exists(getTestDataDir()));
	}
}
