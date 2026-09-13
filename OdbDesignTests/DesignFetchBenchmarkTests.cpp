#include <gtest/gtest.h>
#include "Fixtures/FileArchiveLoadFixture.h"
#include <App/DesignCache.h>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>
#include <system_error>
#include <vector>

using namespace Odb::Lib::App;
using namespace Odb::Test::Fixtures;

namespace Odb::Test
{
	// =========================================================================
	// Design fetch benchmark (M1.4): cold vs warm DesignCache::GetDesign.
	//
	// Driven by scripts/benchmark-design-fetch.sh. Opt-in via
	// ODB_DESIGN_FETCH_BENCH=1 — routine ctest runs skip it. Knobs (all
	// optional, matching the script's arguments):
	//   ODB_BENCH_COLD   - cold iterations, fresh cache each (default 2)
	//   ODB_BENCH_WARM   - warm iterations on a loaded cache (default 10)
	//   ODB_BENCH_DESIGN - design name (archive stem); default = largest
	//                      design archive in the test data directory
	// Emits "[BENCH] <scenario> design=... min/avg/max ms" lines the script's
	// grep consumes; anything else the test prints is diagnostic only.
	// =========================================================================
	class DesignFetchBenchmarkTest : public FileArchiveLoadFixture
	{
	protected:
		static std::uint64_t envIterationCount(const char* name, std::uint64_t fallback)
		{
			const auto* value = std::getenv(name);
			return (value == nullptr || *value == '\0') ? fallback : std::strtoull(value, nullptr, 10);
		}

		// Largest archive in the per-fixture scratch dir (stem = design name,
		// matching DesignCache's scan), or the ODB_BENCH_DESIGN override.
		std::string selectDesignName() const
		{
			const auto* requested = std::getenv("ODB_BENCH_DESIGN");
			if (requested != nullptr && *requested != '\0')
			{
				return requested;
			}

			std::error_code ec;
			std::filesystem::directory_iterator it(m_scratchDir, ec);
			if (ec)
			{
				return "";
			}

			std::string bestName;
			std::uintmax_t bestSize = 0;
			for (const auto& entry : it)
			{
				if (!entry.is_regular_file(ec) || ec)
				{
					ec.clear();
					continue;
				}
				const auto size = entry.file_size(ec);
				if (ec)
				{
					ec.clear();
					continue;
				}
				if (size > bestSize)
				{
					bestSize = size;
					bestName = entry.path().stem().string();
				}
			}
			return bestName;
		}

		static void report(const char* scenario, const std::string& designName, std::vector<double> milliseconds)
		{
			if (milliseconds.empty())
			{
				return;
			}
			std::sort(milliseconds.begin(), milliseconds.end());
			const auto min = milliseconds.front();
			const auto max = milliseconds.back();
			const auto sum = std::accumulate(milliseconds.begin(), milliseconds.end(), 0.0);
			const auto avg = sum / static_cast<double>(milliseconds.size());
			std::cout << "[BENCH] " << scenario << " design=\"" << designName
				<< "\" iters=" << milliseconds.size()
				<< " min=" << min << "ms avg=" << avg << "ms max=" << max << "ms" << std::endl;
		}
	};

	TEST_F(DesignFetchBenchmarkTest, ColdVsWarmDesignFetch)
	{
		if (std::getenv("ODB_DESIGN_FETCH_BENCH") == nullptr)
		{
			GTEST_SKIP() << "benchmark opt-in (ODB_DESIGN_FETCH_BENCH=1; scripts/benchmark-design-fetch.sh sets it)";
			return;
		}
		if (getTestDataDir().empty())
		{
			GTEST_SKIP() << "Test data directory not available (ODB_TEST_DATA_DIR not set)";
			return;
		}

		const auto designName = selectDesignName();
		ASSERT_FALSE(designName.empty()) << "no design archive found in " << m_scratchDir;

		const auto coldIters = envIterationCount("ODB_BENCH_COLD", 2);
		const auto warmIters = envIterationCount("ODB_BENCH_WARM", 10);

		// Cold: a fresh cache per iteration, so every fetch pays the archive
		// read + full parse (the pre-cache cost the response cache removes).
		std::vector<double> coldMs;
		coldMs.reserve(coldIters);
		for (std::uint64_t i = 0; i < coldIters; ++i)
		{
			auto pCache = std::make_unique<DesignCache>(m_scratchDir.string());
			const auto start = std::chrono::steady_clock::now();
			auto pDesign = pCache->GetDesign(designName);
			const auto elapsed = std::chrono::steady_clock::now() - start;
			ASSERT_NE(pDesign, nullptr) << "cold iteration " << i << " failed to load \"" << designName << "\"";
			coldMs.push_back(std::chrono::duration<double, std::milli>(elapsed).count());
		}

		// Warm: one untimed load, then timed cache-hit fetches (lock + map
		// lookup + shared_ptr copy — what every served request after the
		// first pays).
		ASSERT_NE(m_pDesignCache->GetDesign(designName), nullptr) << "warm-up load failed";
		std::vector<double> warmMs;
		warmMs.reserve(warmIters);
		for (std::uint64_t i = 0; i < warmIters; ++i)
		{
			const auto start = std::chrono::steady_clock::now();
			auto pDesign = m_pDesignCache->GetDesign(designName);
			const auto elapsed = std::chrono::steady_clock::now() - start;
			ASSERT_NE(pDesign, nullptr);
			warmMs.push_back(std::chrono::duration<double, std::milli>(elapsed).count());
		}

		report("cold", designName, std::move(coldMs));
		report("warm", designName, std::move(warmMs));
	}
}
