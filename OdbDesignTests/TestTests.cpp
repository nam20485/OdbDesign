#include <gtest/gtest.h>
#include "Fixtures/FileArchiveLoadFixture.h"
#include <filesystem>

using namespace std::filesystem;
using namespace Odb::Test::Fixtures;

namespace Odb::Test
{
	// Demonstrate some basic assertions.
	TEST(TestTest, BasicAssertions)
	{
		// Expect two strings not to be equal.
		EXPECT_STRNE("hello", "world");
		// Expect equality.
		EXPECT_EQ(7 * 6, 42);
		EXPECT_FLOAT_EQ(1.0f, 1.0f);
		EXPECT_TRUE(true != false);
		EXPECT_FALSE(true == false);
	}

	TEST(TestTest, SucceedSucceeds)
	{
		SUCCEED();
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
