#include <gtest/gtest.h>
#include "Fixtures/FileArchiveLoadFixture.h"
#include <App/DesignCache.h>
#include "ProductModel/Design.h"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <map>
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

	// =========================================================================
	// M2.4 size scenarios: the always-on connectivity payload growth, plus the
	// outstanding M1.2 feature-UID wire delta. Opt-in with the benchmark
	// (ODB_DESIGN_FETCH_BENCH=1); scripts/benchmark-design-fetch.sh greps the
	// [BENCH] lines. The default-on 150 KB sample_design gate lives in
	// ConnectivityContractTests (always available to CI); the 10%-of-fileModel
	// acceptance on the 81k-component legacy design runs here, where the
	// gitignored local corpus is present.
	// =========================================================================
	namespace
	{
		void clearFeatureIds(Odb::Lib::Protobuf::FeaturesFile* pFeaturesFile, std::uint64_t& clearedCount)
		{
			if (pFeaturesFile == nullptr) return;
			for (auto& record : *pFeaturesFile->mutable_featurerecords())
			{
				if (record.has_id())
				{
					record.clear_id();
					++clearedCount;
				}
			}
		}
	}

	TEST_F(DesignFetchBenchmarkTest, ConnectivityPayloadSizes)
	{
		if (std::getenv("ODB_DESIGN_FETCH_BENCH") == nullptr)
		{
			GTEST_SKIP() << "benchmark opt-in (ODB_DESIGN_FETCH_BENCH=1)";
			return;
		}
		if (getTestDataDir().empty())
		{
			GTEST_SKIP() << "Test data directory not available (ODB_TEST_DATA_DIR not set)";
			return;
		}

		struct Spec
		{
			const char* designName;
			bool fromTestData;
		};
		static const std::map<std::string, std::string> localArchives = {
			{ "Panel-g7162-31800_odb", "new_designs/Panel - g7162-31800_odb.tgz" },
			{ "350-41017_rev1_odbjob_v7", "new_designs/350-41017_rev1_odbjob_v7.tgz" },
		};

		const std::vector<Spec> specs = {
			{ "sample_design", true },
			{ "Panel-g7162-31800_odb", false },
			{ "350-41017_rev1_odbjob_v7", false },
		};

		for (const auto& spec : specs)
		{
			std::filesystem::path archive;
			if (spec.fromTestData)
			{
				archive = getIsolatedDesignPath("sample_design.tgz");
			}
			else
			{
				const auto source = std::filesystem::path(ODBDESIGN_SOURCE_DIR) / localArchives.at(spec.designName);
				std::error_code ec;
				if (std::filesystem::is_regular_file(source, ec))
				{
					const auto copy = m_scratchDir / source.filename();
					std::filesystem::copy_file(source, copy, std::filesystem::copy_options::overwrite_existing, ec);
					if (!ec) archive = copy;
				}
			}
			if (archive.empty() || !std::filesystem::exists(archive))
			{
				std::cout << "[BENCH] size design=\"" << spec.designName << "\" skipped (archive not present)" << std::endl;
				continue;
			}

			auto pDesign = std::make_shared<Odb::Lib::ProductModel::Design>();
			ASSERT_TRUE(pDesign->Build(archive.string())) << spec.designName << ": build failed";
			const auto pMsg = pDesign->to_protobuf(/*includeNormalizedLists=*/false);

			const auto total = pMsg->ByteSizeLong();
			const auto connectivity = pMsg->connectivity().ByteSizeLong();
			const auto fileModel = pMsg->filemodel().ByteSizeLong();

			// M1.2 delta: exact wire bytes of FeatureRecord.id values, measured
			// by re-serializing a copy with every feature id cleared.
			auto fmCopy = pMsg->filemodel();
			std::uint64_t clearedFeatureIds = 0;
			for (auto& stepEntry : *fmCopy.mutable_stepsbyname())
			{
				auto& step = stepEntry.second;
				clearFeatureIds(step.has_profilefile() ? step.mutable_profilefile() : nullptr, clearedFeatureIds);
				for (auto& layerEntry : *step.mutable_layersbyname())
				{
					clearFeatureIds(layerEntry.second.has_featurefile() ? layerEntry.second.mutable_featurefile() : nullptr, clearedFeatureIds);
				}
			}
			const auto featureUidDelta = fileModel - fmCopy.ByteSizeLong();

			std::cout << "[BENCH] size design=\"" << spec.designName
				<< "\" total=" << total
				<< " connectivity=" << connectivity
				<< " fileModel=" << fileModel
				<< " connPctOfFileModel=" << (fileModel > 0 ? (100.0 * static_cast<double>(connectivity)) / static_cast<double>(fileModel) : 0.0)
				<< "% featureUidDelta=" << featureUidDelta
				<< " (" << clearedFeatureIds << " ids)" << std::endl;

			if (std::string(spec.designName) == "350-41017_rev1_odbjob_v7")
			{
				// M2.4 merge gate: connectivity <= 10% of the fileModel payload
				// on the 81,157-component legacy design.
				EXPECT_LE(connectivity, fileModel / 10)
					<< "connectivity exceeds 10% of fileModel on 350-41017 (D4 size gate)";
			}
		}
	}
}
