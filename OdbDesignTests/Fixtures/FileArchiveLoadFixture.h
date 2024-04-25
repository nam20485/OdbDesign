#pragma once

#include "gtest/gtest.h"
#include <filesystem>
#include "OdbDesign.h"
#include <memory>
#include <string>
#include <vector>

namespace Odb::Test::Fixtures
{
	class FileArchiveLoadFixture : public testing::Test
	{
	public:		
		FileArchiveLoadFixture();

	protected:
		std::filesystem::path m_testDataDir;
		std::unique_ptr<Odb::Lib::App::DesignCache> m_pDesignCache;
		
		const bool m_removeDecompressedDirectories = true;

		void SetUp() override;
		void TearDown() override;

		static std::filesystem::path getTestDataDir();
		std::filesystem::path getTestDataFilesDir();
		std::filesystem::path getDesignPath(const std::string& filename) const;
		std::filesystem::path getTestDataFilePath(const std::string& filename) const;

		static inline const std::vector<std::string> KEEP_DIRECTORIES = { "files" };

		static inline constexpr const char ODB_TEST_DATA_DIR_ENV_NAME[] = "ODB_TEST_DATA_DIR";

		static inline constexpr const char TESTFILE_DIR[] = "files";

		static inline constexpr bool ENABLE_TEST_LOGGING = false;

	};
}
