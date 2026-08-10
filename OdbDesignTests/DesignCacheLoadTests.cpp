#include <gtest/gtest.h>
#include "Fixtures/FileArchiveLoadFixture.h"
#include "OdbDesign.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <system_error>

#include "ArchiveExtractor.h"

//using namespace Odb::Lib::App;
using namespace Odb::Lib::FileModel;
using namespace Odb::Test::Fixtures;
using namespace std::filesystem;

namespace
{
	// Isolated, symlink-resolved temp directory so libarchive extraction works
	// on macOS (where temp_directory_path() begins with /var -> /private/var).
	std::filesystem::path makeUniqueCacheDir()
	{
		std::error_code ec;
		const auto tempDir = std::filesystem::weakly_canonical(std::filesystem::temp_directory_path(ec), ec);
		const auto base = (ec ? std::filesystem::temp_directory_path() : tempDir) / "odb_designcache_loadtests";
		std::filesystem::create_directories(base, ec);
		std::random_device rd;
		return base / (std::to_string(rd()) + "_" + std::to_string(rd()));
	}
}

namespace Odb::Test
{
	TEST_F(FileArchiveLoadFixture, Load_Design_Succeeds_sample_design_tgz)
	{
		//ASSERT_TRUE(exists(getDesignPath("sample_design.tgz")));
		auto pDesign = m_pDesignCache->GetDesign("sample_design");
		ASSERT_NE(pDesign, nullptr);
	}

	TEST_F(FileArchiveLoadFixture, Load_Design_Succeeds_designodb_rigidflex_tgz)
	{
		//ASSERT_TRUE(exists(getDesignPath("designodb_rigidflex.tgz")));
		auto pDesign = m_pDesignCache->GetDesign("designodb_rigidflex");
		ASSERT_NE(pDesign, nullptr);
	}

	// A present-but-unextractable archive must surface as an error (throw),
	// not a misleading nullptr that maps to NOT_FOUND/404.
	TEST(DesignCacheLoadError, ThrowsForUnextractableArchive)
	{
		const auto cacheDir = makeUniqueCacheDir();
		std::error_code ec;
		std::filesystem::create_directories(cacheDir, ec);
		ASSERT_FALSE(ec) << ec.message();

		// Garbage bytes that look like an archive by filename but cannot extract.
		const auto archivePath = cacheDir / "broken.tgz";
		{
			std::ofstream out(archivePath, std::ios::binary);
			out << "this is not a valid archive";
		}

		Odb::Lib::App::DesignCache cache(cacheDir.string());
		EXPECT_THROW(cache.GetFileArchive("broken"), std::exception);

		std::filesystem::remove_all(cacheDir, ec);
	}

	// A valid archive that extracts but fails to parse must also throw
	// (ParseFileModel already throws on parse failure; this guards the contract).
	TEST(DesignCacheLoadError, ThrowsForUnparseableArchive)
	{
		const auto cacheDir = makeUniqueCacheDir();
		std::error_code ec;
		std::filesystem::create_directories(cacheDir, ec);
		ASSERT_FALSE(ec) << ec.message();

		// Valid tgz with the required top-level dirs but empty, so it extracts
		// but ParseDesignDirectory fails.
		auto srcRoot = cacheDir / "src" / "bad_design";
		for (const auto* sub : { "fonts", "misc", "matrix", "steps" })
		{
			std::filesystem::create_directories(srcRoot / sub, ec);
			ASSERT_FALSE(ec) << ec.message();
		}

		std::string createdArchivePath;
		ASSERT_TRUE(::Utils::ArchiveExtractor::CompressDir(
			srcRoot.string(), cacheDir.string(), "bad_design", createdArchivePath));
		ASSERT_FALSE(createdArchivePath.empty());

		Odb::Lib::App::DesignCache cache(cacheDir.string());
		EXPECT_THROW(cache.GetFileArchive("bad_design"), std::exception);

		std::filesystem::remove_all(cacheDir, ec);
	}

	// A genuinely absent design must still return nullptr (no throw), so callers
	// can keep mapping it to NOT_FOUND.
	TEST(DesignCacheLoadError, ReturnsNullptrForMissingDesign)
	{
		const auto cacheDir = makeUniqueCacheDir();
		std::error_code ec;
		std::filesystem::create_directories(cacheDir, ec);
		ASSERT_FALSE(ec) << ec.message();

		Odb::Lib::App::DesignCache cache(cacheDir.string());
		EXPECT_NO_THROW({
			auto p = cache.GetFileArchive("does_not_exist");
			EXPECT_EQ(p, nullptr);
		});

		std::filesystem::remove_all(cacheDir, ec);
	}
}