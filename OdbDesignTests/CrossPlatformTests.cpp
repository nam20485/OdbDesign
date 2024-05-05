#include <gtest/gtest.h>
#include "CrossPlatform.h"
#include <ctime>
#include "Fixtures/TestDataFixture.h"
#include <string>
#include <filesystem>

using namespace std::filesystem;
using namespace Odb::Test::Fixtures;
using namespace testing;
using namespace Utils;

namespace Odb::Test
{
	TEST_F(TestDataFixture, Test_CrossPlatform_GetEnvSafe_VariableExists)
	{
		std::string value;
		ASSERT_EQ(value.size(), 0U);
		ASSERT_TRUE(CrossPlatform::getenv_safe(ODB_TEST_DATA_DIR_ENV_NAME, value));
		ASSERT_STRNE(value.c_str(), "");
		ASSERT_STREQ(path(value).c_str(), m_testDataDir.c_str());
	}

	TEST_F(TestDataFixture, Test_CrossPlatform_GetEnvSafe_VariableDoesntExist)
	{
		std::string value;
		ASSERT_EQ(value.size(), 0U);
		ASSERT_FALSE(CrossPlatform::getenv_safe("FEARISTHEMINDKILLER", value));
		ASSERT_STREQ(value.c_str(), "");		
	}

	TEST_F(TestDataFixture, Test_CrossPlatform_TmpNameSafe_ReturnsRandomFilename)
	{
		std::string value1;
		ASSERT_EQ(value1.size(), 0U);
		ASSERT_TRUE(CrossPlatform::tmpnam_safe(value1));
		ASSERT_STRNE(value1.c_str(), "");

		std::string value2;
		ASSERT_EQ(value2.size(), 0U);
		ASSERT_TRUE(CrossPlatform::tmpnam_safe(value2));
		ASSERT_STRNE(value2.c_str(), "");

		ASSERT_STRNE(value1.c_str(), value2.c_str());
	}

	TEST_F(TestDataFixture, Test_CrossPlatform_LocalTimeSafe_ReturnsSomeTime)
	{
		time_t tt{ 0 };
		ASSERT_EQ(tt, 0LL);
		std::time(&tt);
		ASSERT_NE(tt, 0LL);

		struct tm tm{ 0 };

		ASSERT_EQ(tm.tm_year, 0);
		ASSERT_EQ(tm.tm_mon, 0);
		ASSERT_EQ(tm.tm_mday, 0);
		ASSERT_EQ(tm.tm_hour, 0);
		ASSERT_EQ(tm.tm_min, 0);
		ASSERT_EQ(tm.tm_sec, 0);

		ASSERT_TRUE(CrossPlatform::localtime_safe(&tt, tm));

		ASSERT_NE(tm.tm_year, 0);
		ASSERT_NE(tm.tm_mon, 0);
		ASSERT_NE(tm.tm_mday, 0);
		ASSERT_NE(tm.tm_hour, 0);
		ASSERT_NE(tm.tm_min, 0);
		ASSERT_NE(tm.tm_sec, 0);
	}
}