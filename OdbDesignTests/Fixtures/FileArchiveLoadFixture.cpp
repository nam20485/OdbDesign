#include "FileArchiveLoadFixture.h"
#include <string>
#include "Logger.h"

using namespace std::filesystem;
//using namespace Odb::Lib;
using namespace Odb::Lib::App;
using namespace Utils;

namespace Odb::Test::Fixtures
{
	FileArchiveLoadFixture::FileArchiveLoadFixture()
		: m_testDataDir()
		, m_pDesignCache(nullptr)
	{		
	}

	void FileArchiveLoadFixture::SetUp()
	{
		//Logger::instance()->start();
		
		ASSERT_FALSE(getTestDataDir().empty());		
		m_testDataDir = getTestDataDir();		
		m_testDataDir = m_testDataDir.make_preferred();

		ASSERT_TRUE(exists(m_testDataDir));

		m_pDesignCache = std::unique_ptr<DesignCache>(new DesignCache(m_testDataDir.string()));
		ASSERT_NE(m_pDesignCache, nullptr);
	}

	/*static*/ std::string FileArchiveLoadFixture::getTestDataDir()
	{
		auto szTestDataDir = std::getenv(ODB_TEST_DATA_DIR_ENV_NAME);
		if (szTestDataDir == nullptr) return "";
		//if (szTestDataDir == nullptr) throw std::runtime_error("ODB_TEST_DATA_DIR environment variable is not set");				
		//if (!exists(szTestDataDir)) throw std::runtime_error("ODB_TEST_DATA_DIR environment variable is set to a non-existent directory");
		return szTestDataDir;
	}

	void FileArchiveLoadFixture::TearDown()
	{		
		if (m_removeDecompressedDirectories)
		{		
			if (exists(m_testDataDir))
			{
				// delete uncompressed directories
				for (const auto& entry : directory_iterator(m_testDataDir))
				{
					if (is_directory(entry))
					{
						remove_all(entry.path());
					}
				}
			}
		}

		//Logger::instance()->stop();		
	}

	path FileArchiveLoadFixture::getDesignPath(const std::string& filename) const
	{
		return m_testDataDir / filename;
	}
}