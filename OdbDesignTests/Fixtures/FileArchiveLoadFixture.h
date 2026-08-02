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

		// Per-fixture isolated directory: design archives are copied here and extracted
		// within it, so concurrent test processes never share a mutable on-disk location.
		std::filesystem::path m_scratchDir;

		void SetUp() override;
		void TearDown() override;

		std::filesystem::path getDesignPath(const std::string& filename) const;

		// Returns the path to a per-fixture isolated copy of a design archive.
		// Use this (instead of getDesignPath) when a test extracts/parses an archive
		// directly, so extraction lands in the per-fixture scratch dir rather than the
		// shared test-data directory (keeps parallel test runs from racing on disk).
		std::filesystem::path getIsolatedDesignPath(const std::string& filename) const;

	};
}
