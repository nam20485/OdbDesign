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
#include <stdexcept>
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
		if (ec)
		{
			throw std::runtime_error("failed to create scratch base dir '" + base.string() + "': " + ec.message());
		}

		static std::atomic<uint64_t> counter{ 0 };
		std::random_device rd;
		std::mt19937_64 gen((static_cast<uint64_t>(rd()) << 32) ^ rd());
		std::uniform_int_distribution<uint64_t> dist;

		// Bounded retry: create_directory atomically succeeds only if the dir does not
		// yet exist. Collisions are astronomically unlikely (64-bit random + atomic
		// counter), but a bound makes the failure mode explicit rather than spinning
		// forever if the filesystem is in a pathological state.
		constexpr int kMaxAttempts = 100;
		for (int attempt = 0; attempt < kMaxAttempts; ++attempt)
		{
			std::ostringstream name;
			name << std::hex << dist(gen) << "_" << std::hex << counter.fetch_add(1);

			auto candidate = base / name.str();
			if (std::filesystem::create_directory(candidate, ec))
			{
				// Resolve any symlinks in the system temp path (e.g. macOS /var ->
				// /private/var) so libarchive's ARCHIVE_EXTRACT_SECURE_SYMLINKS check
				// does not refuse to extract into the scratch directory.
				auto resolved = std::filesystem::weakly_canonical(candidate, ec);
				return ec ? candidate : resolved;
			}
		}

		throw std::runtime_error("failed to create a unique scratch dir after " + std::to_string(kMaxAttempts) + " attempts");
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
		directory_iterator testDataIt(getTestDataDir(), ec);
		// Fail loudly if the test-data directory could not be iterated; otherwise
		// the loop below silently copies nothing and the test later fails with an
		// opaque parse error instead of a clear setup failure.
		ASSERT_FALSE(ec) << "directory_iterator failed for '" << getTestDataDir() << "': " << ec.message();
		for (const auto& entry : testDataIt)
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
			// Fail loudly if an archive failed to copy, otherwise the DesignCache is
			// pointed at a scratch dir missing the archive and the test later fails
			// with an opaque parse error rather than a clear setup failure.
			ASSERT_FALSE(ec) << "copy_file failed for '" << entry.path() << "': " << ec.message();
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
