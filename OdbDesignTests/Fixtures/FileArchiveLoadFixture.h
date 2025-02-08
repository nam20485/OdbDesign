#pragma once

#include "gtest/gtest.h"
#include <filesystem>
#include <string>
#include <App/DesignCache.h>
#include "TestDataFixture.h"

namespace Odb::Test::Fixtures
{
	class FileArchiveLoadFixture : public TestDataFixture
	{
	public:		
		FileArchiveLoadFixture();

	protected:
		std::unique_ptr<Odb::Lib::App::DesignCache> m_pDesignCache;
		
		const bool m_removeDecompressedDirectories = true;

		void SetUp() override;
		void TearDown() override;

		std::filesystem::path getDesignPath(const std::string& filename) const;
			
		static inline const std::vector<std::string> KEEP_DIRECTORIES = { TESTDATA_FILES_DIR };		

	};
}
