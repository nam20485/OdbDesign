#include "FileArchiveLoadFixture.h"
#include <string>

using namespace std::filesystem;
using namespace Odb::Lib;
using namespace Odb::Lib::App;

namespace Odb::Test
{
	FileArchiveLoadFixture::FileArchiveLoadFixture()
		: m_testDataDir()
		, m_pDesignCache(nullptr)

	{		
	}

	void FileArchiveLoadFixture::SetUp()
	{				
		ASSERT_FALSE(getTestDataDir().empty());
				
		m_testDataDir = getTestDataDir();
		ASSERT_TRUE(exists(m_testDataDir));

		m_pDesignCache = std::unique_ptr<DesignCache>(new DesignCache(m_testDataDir.string()));
		ASSERT_NE(m_pDesignCache, nullptr);
	}

	std::string FileArchiveLoadFixture::getTestDataDir()
	{
		auto szTestDataDir = std::getenv("ODB_TEST_DATA_DIR");
		if (szTestDataDir == nullptr) throw std::runtime_error("ODB_TEST_DATA_DIR environment variable is not set");				
		if (!exists(szTestDataDir)) throw std::runtime_error("ODB_TEST_DATA_DIR environment variable is set to a non-existent directory");
		return szTestDataDir;
	}

	void FileArchiveLoadFixture::TearDown()
	{		
		if (m_removeDecompressedDirectories)
		{
			for (const auto& loadedName : m_pDesignCache->getLoadedFileArchiveNames())
			{
				if (is_directory(getDesignPath(loadedName)))
				{
					remove_all(getDesignPath(loadedName));
				}
			}

			// delete uncompressed directories
			//for (const auto& entry : directory_iterator(m_testDataDir))
			//{
			//	if (is_directory(entry))
			//	{
			//		remove_all(entry.path());
			//	}
			//}
		}
	}

	path FileArchiveLoadFixture::getDesignPath(const std::string& filename) const
	{
		return m_testDataDir / filename;
	}
}