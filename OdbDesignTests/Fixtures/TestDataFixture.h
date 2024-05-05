#pragma once

#include "gtest/gtest.h"
#include <filesystem>
#include <string>

namespace Odb::Test::Fixtures
{
	class TestDataFixture : public testing::Test
	{
	public:
		TestDataFixture();

	protected:
		std::filesystem::path m_testDataDir;		

		const bool m_removeDecompressedDirectories = true;

		virtual void SetUp() override;
		virtual void TearDown() override;

		static std::filesystem::path getTestDataDir();
		std::filesystem::path getTestDataFilesDir() const;
		std::filesystem::path getTestDataFilePath(const std::string& filename) const;

		static inline constexpr const char ODB_TEST_DATA_DIR_ENV_NAME[] = "ODB_TEST_DATA_DIR";
		static inline constexpr const char TESTDATA_FILES_DIR[] = "FILES";		

		static inline constexpr bool ENABLE_TEST_LOGGING = false;

	};
}