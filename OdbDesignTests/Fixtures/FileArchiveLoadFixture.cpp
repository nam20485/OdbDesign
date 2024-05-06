#include "FileArchiveLoadFixture.h"
#include <string>
#include "Logger.h"
#include <memory>
#include <cstdlib>
#include "App/DesignCache.h"
#include "TestDataFixture.h"
#include <filesystem>

using namespace std::filesystem;
//using namespace Odb::Lib;
using namespace Odb::Lib::App;
using namespace Utils;

namespace Odb::Test::Fixtures
{
	FileArchiveLoadFixture::FileArchiveLoadFixture()
		: m_pDesignCache(nullptr)
	{		
	}

	void FileArchiveLoadFixture::SetUp()
	{
		TestDataFixture::SetUp();

		m_pDesignCache = std::unique_ptr<DesignCache>(new DesignCache(getTestDataDir().string()));
		ASSERT_NE(m_pDesignCache, nullptr);
	}

	void FileArchiveLoadFixture::TearDown()
	{
		if (m_removeDecompressedDirectories)
		{
			if (exists(getTestDataDir()))
			{
				// delete uncompressed directories
				for (const auto& entry : directory_iterator(getTestDataDir()))
				{
					if (is_directory(entry))
					{
						if (std::find(KEEP_DIRECTORIES.begin(), KEEP_DIRECTORIES.end(), entry.path().filename()) == KEEP_DIRECTORIES.end())
						{
							remove_all(entry.path());
						}
					}
				}
			}
		}

		TestDataFixture::TearDown();
	}	

	path FileArchiveLoadFixture::getDesignPath(const std::string& filename) const
	{
		return getTestDataDir() / filename;
	}
}