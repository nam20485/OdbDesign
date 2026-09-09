#include "TestDataFixture.h"
#include "CrossPlatform.h"
#include <string>
#include <Logger.h>
#include <filesystem>
#include <cstdlib>

using namespace std::filesystem;
using namespace Utils;

namespace Odb::Test::Fixtures
{
	TestDataFixture::TestDataFixture()
		: m_testDataDir()
	{
	}

	void TestDataFixture::SetUp()
	{
		if (ENABLE_TEST_LOGGING)
		{
			Logger::instance()->start();
		}

		ASSERT_FALSE(getTestDataDir().empty());
		m_testDataDir = getTestDataDir();
		m_testDataDir = m_testDataDir.make_preferred();
		ASSERT_TRUE(exists(m_testDataDir));
	}

	void TestDataFixture::TearDown()
	{
		if (ENABLE_TEST_LOGGING)
		{
			Logger::instance()->stop();
		}
	}

	path TestDataFixture::getTestDataDir()
	{
		static const path testDataDir = []()
		{
			std::string envValue;
			if (!CrossPlatform::getenv_safe(ODB_TEST_DATA_DIR_ENV_NAME, envValue))
			{
				return path{};
			}

			auto resolvedPath = path(envValue);
			resolvedPath.make_preferred();
			return resolvedPath;
		}();

		return testDataDir;
	}

	path TestDataFixture::getTestDataFilesDir() const
	{
		return m_testDataDir / TESTDATA_FILES_DIR;
	}	

	path TestDataFixture::getTestDataFilePath(const std::string& filename) const
	{
		return getTestDataFilesDir() / filename;
	}
}
