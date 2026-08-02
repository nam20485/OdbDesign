#include "FileArchiveLoadFixture.h"
#include <string>
#include "Logger.h"
#include <memory>
#include <cstdlib>
#include "App/DesignCache.h"
#include "TestDataFixture.h"
#include "ArchiveExtractor.h"
#include <filesystem>
#include <random>
#include <atomic>
#include <sstream>
#include <system_error>
#include <cstdint>

using namespace std::filesystem;
using namespace Utils;
using namespace Odb::Lib::App;

namespace
{
	// Creates a unique, per-fixture scratch directory under the system temp dir.
	// Each test fixture instance gets its own directory so concurrent test processes
	// never extract into or delete from a shared on-disk location.
	std::filesystem::path createUniqueScratchDir()
	{
		std::error_code ec;
		auto base = std::filesystem::temp_directory_path() / "odbdesign_tests";
		std::filesystem::create_directories(base, ec);

		static std::atomic<uint64_t> counter{ 0 };
		std::random_device rd;
		std::mt19937_64 gen((static_cast<uint64_t>(rd()) << 32) ^ rd());
		std::uniform_int_distribution<uint64_t> dist;

		for (;;)
		{
			std::ostringstream name;
			name << std::hex << dist(gen) << "_" << std::hex << counter.fetch_add(1);

			auto candidate = base / name.str();
			// create_directory atomically succeeds only if the dir does not yet exist.
			if (std::filesystem::create_directory(candidate, ec))
			{
				return candidate;
			}
		}
	}
}

namespace Odb::Test::Fixtures
{
	FileArchiveLoadFixture::FileArchiveLoadFixture()
		: m_pDesignCache(nullptr)
	{
	}

	void FileArchiveLoadFixture::SetUp()
	{
		TestDataFixture::SetUp();

		m_scratchDir = createUniqueScratchDir();

		// Copy the design archives into the isolated scratch dir so that extraction
		// (which writes next to the archive) happens per-fixture instead of in the
		// shared test-data directory. The shared directory is treated as read-only.
		std::error_code ec;
		for (const auto& entry : directory_iterator(getTestDataDir(), ec))
		{
			if (!entry.is_regular_file())
			{
				continue;
			}
			if (!ArchiveExtractor::IsArchiveTypeSupported(entry.path().filename()))
			{
				continue;
			}

			std::filesystem::copy_file(entry.path(), m_scratchDir / entry.path().filename(),
				std::filesystem::copy_options::overwrite_existing, ec);
		}

		m_pDesignCache = std::unique_ptr<DesignCache>(new DesignCache(m_scratchDir.string()));
		ASSERT_NE(m_pDesignCache, nullptr);
	}

	void FileArchiveLoadFixture::TearDown()
	{
		// Remove only this fixture's isolated scratch dir; never touch the shared
		// test-data directory (doing so raced with concurrent test processes).
		if (!m_scratchDir.empty())
		{
			std::error_code ec;
			std::filesystem::remove_all(m_scratchDir, ec);
		}

		TestDataFixture::TearDown();
	}

	path FileArchiveLoadFixture::getDesignPath(const std::string& filename) const
	{
		return getTestDataDir() / filename;
	}

	path FileArchiveLoadFixture::getIsolatedDesignPath(const std::string& filename) const
	{
		return m_scratchDir / filename;
	}
}
