#include <gtest/gtest.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

#if defined(__GLIBC__)
#include <malloc.h>
#endif

#include "Fixtures/FileArchiveLoadFixture.h"
#include "OdbDesignServer/Services/OdbDesignServiceImpl.h"
#include "OdbDesignServer/Config/GrpcServiceConfig.h"
#include <design.pb.h>
#include <service.grpc.pb.h>

using namespace Odb::Lib::App;
using namespace Odb::Test::Fixtures;
using namespace OdbDesignServer::Services;
using namespace OdbDesignServer::Config;
using namespace Odb::Grpc;

namespace Odb::Test
{
	// =========================================================================
	// M6 memory + perf gates (getdesign-streaming-oom-plan.md §M6.5).
	//
	// Opt-in via ODB_DESIGN_STREAM_BENCH=1 (same convention as
	// DesignFetchBenchmarkTests); the two large design archives live outside
	// the CI corpus (gitignored new_designs/ at the repo root) — each spec
	// reports "skipped" when its archive is absent.
	//
	// Serves each large design through a real in-process gRPC server:
	//   - cold GetDesignStream (parse + walk + spool tee) — peak RSS must stay
	//     within the ~4 GiB budget (VmHWM high-water mark from /proc/self/status),
	//   - monolithic GetDesign must come back RESOURCE_EXHAUSTED (the OOM path
	//     is now a clean error),
	//   - warm GetDesignStream served from the disk spool.
	// Also times sample_design monolithic vs cold/warm streaming (perf gate:
	// cold within ~1.5x monolithic, warm <= monolithic).
	// =========================================================================
	class DesignStreamMemoryTest : public FileArchiveLoadFixture
	{
	protected:
		void SetUp() override
		{
			if (std::getenv("ODB_DESIGN_STREAM_BENCH") == nullptr)
			{
				GTEST_SKIP() << "stream benchmark opt-in (ODB_DESIGN_STREAM_BENCH=1)";
				return;
			}
			if (getTestDataDir().empty())
			{
				GTEST_SKIP() << "Test data directory not available (ODB_TEST_DATA_DIR not set)";
				return;
			}
			FileArchiveLoadFixture::SetUp();
		}

		// One isolated serve environment: a directory holding exactly one
		// archive, a DesignCache over it, the service, and an in-process gRPC
		// server + stub. Torn down per design so RSS from one design never
		// leaks into the other's measurement.
		struct ServeEnv
		{
			std::shared_ptr<DesignCache> cache;
			std::shared_ptr<GrpcServiceConfig> config;
			std::unique_ptr<OdbDesignServiceImpl> service;
			std::unique_ptr<grpc::Server> server;
			std::unique_ptr<OdbDesignService::Stub> stub;

			ServeEnv() = default;
			~ServeEnv()
			{
				if (server != nullptr)
				{
					server->Shutdown();
				}
			}
			ServeEnv(ServeEnv&&) noexcept = default;
			ServeEnv& operator=(ServeEnv&&) noexcept = default;
		};

		void MakeServeEnv(ServeEnv& env, std::shared_ptr<DesignCache> cache, std::uint64_t monolithicBudget)
		{
			env.cache = std::move(cache);
			env.config = GrpcServiceConfig::CreateDefault();
			env.service = std::make_unique<OdbDesignServiceImpl>(env.cache, env.config);

			grpc::ServerBuilder builder;
			int port = 0;
			builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &port);
			builder.RegisterService(env.service.get());
			env.server = builder.BuildAndStart();
			EXPECT_NE(env.server, nullptr);
			EXPECT_GT(port, 0);
			env.stub = OdbDesignService::NewStub(MakeChannel(port));

			// 0 leaves the real default budget; nonzero overrides it.
			if (monolithicBudget != 0)
			{
				env.cache->setMonolithicWireBudget(monolithicBudget);
			}
		}

		struct ConsumeResult
		{
			bool ok = false;
			std::string error;
			std::uint64_t chunks = 0;
			std::uint64_t wireBytes = 0;       // sum of serialized chunk sizes
			double milliseconds = 0.0;
		};

		// Consumes the stream chunk-by-chunk WITHOUT reassembling (the memory
		// gate measures the serve path; reassembling would build the very
		// multi-GB tree the fix removes).
		ConsumeResult ConsumeStream(OdbDesignService::Stub& stub, const std::string& designName,
			bool includeNormalizedLists = false)
		{
			grpc::ClientContext ctx;
			GetDesignRequest req;
			req.set_design_name(designName);
			req.set_include_normalized_lists(includeNormalizedLists);
			auto reader = stub.GetDesignStream(&ctx, req);

			ConsumeResult result;
			const auto start = std::chrono::steady_clock::now();
			DesignChunk chunk;
			while (reader->Read(&chunk))
			{
				++result.chunks;
				result.wireBytes += static_cast<std::uint64_t>(chunk.ByteSizeLong());
				chunk.Clear();
			}
			result.milliseconds = std::chrono::duration<double, std::milli>(
				std::chrono::steady_clock::now() - start).count();
			const auto status = reader->Finish();
			result.ok = status.ok();
			result.error = status.error_message();
			return result;
		}

		grpc::Status FetchMonolithicStatus(OdbDesignService::Stub& stub, const std::string& designName,
			std::uint64_t* bytesOut = nullptr)
		{
			grpc::ClientContext ctx;
			GetDesignRequest req;
			req.set_design_name(designName);
			Odb::Lib::Protobuf::ProductModel::Design resp;
			const auto status = stub.GetDesign(&ctx, req, &resp);
			if (bytesOut != nullptr && status.ok())
			{
				*bytesOut = resp.ByteSizeLong();
			}
			return status;
		}

		// ---- /proc instrumentation (Linux) ----

		static bool readStatusField(const char* field, std::uint64_t& valueOut_kb)
		{
			std::ifstream status("/proc/self/status");
			std::string line;
			while (std::getline(status, line))
			{
				if (line.rfind(field, 0) == 0)
				{
					// "VmHWM:\t  123456 kB"
					std::istringstream ss(line.substr(strlen(field)));
					ss >> valueOut_kb;
					return true;
				}
			}
			return false;
		}

		static std::uint64_t peakRssKb()
		{
			std::uint64_t kb = 0;
			readStatusField("VmHWM:", kb);
			return kb;
		}

		static std::uint64_t currentRssKb()
		{
			std::uint64_t kb = 0;
			readStatusField("VmRSS:", kb);
			return kb;
		}

		// Resets the high-water mark so each design's peak is attributable to
		// that design alone (Linux clear_refs 5; silently ignored elsewhere).
		static void tryResetPeakRss()
		{
			std::ofstream clearRefs("/proc/self/clear_refs");
			clearRefs << "5";
		}

		std::filesystem::path copyBigDesign(const std::string& archiveName, const std::string& designName)
		{
			const auto source = std::filesystem::path(ODBDESIGN_SOURCE_DIR) / "new_designs" / archiveName;
			std::error_code ec;
			if (!std::filesystem::is_regular_file(source, ec))
			{
				std::cout << "[BENCH] stream design=\"" << designName << "\" skipped (archive not present: "
					<< source.string() << ")" << std::endl;
				return {};
			}
			const auto dir = m_scratchDir / ("serve_" + designName);
			std::filesystem::create_directories(dir, ec);
			const auto copy = dir / archiveName;
			std::filesystem::copy_file(source, copy, std::filesystem::copy_options::overwrite_existing, ec);
			if (ec)
			{
				std::cout << "[BENCH] stream design=\"" << designName << "\" skipped (copy failed: "
					<< ec.message() << ")" << std::endl;
				return {};
			}
			return dir;
		}

		static std::shared_ptr<grpc::Channel> MakeChannel(int port)
		{
			// Mirror the production server's raised message limit; the default
			// 4 MB client cap rejects sample_design's ~21 MB monolithic response.
			grpc::ChannelArguments args;
			args.SetMaxReceiveMessageSize(256 * 1024 * 1024);
			return grpc::CreateCustomChannel("127.0.0.1:" + std::to_string(port),
				grpc::InsecureChannelCredentials(), args);
		}

		static bool waitForCondition(const std::function<bool()>& predicate, std::chrono::milliseconds timeout)
		{
			const auto deadline = std::chrono::steady_clock::now() + timeout;
			while (std::chrono::steady_clock::now() < deadline)
			{
				if (predicate())
				{
					return true;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
			return predicate();
		}
	// One large design, served cold -> (monolithic rejection) -> warm, in a
	// single process. Peak RSS is the /proc VmHWM high-water mark, reset at
	// scenario start (Linux clear_refs); glibc-retained heap from earlier
	// work in the process is trimmed at teardown so a following scenario in
	// the same process starts from a settled baseline. Gate numbers should be
	// recorded from one-process-per-design invocations (see PR evidence).
		void RunLargeDesignScenario(const char* archiveName, const char* designName)
		{
			const auto dir = copyBigDesign(archiveName, designName);
			if (dir.empty())
			{
				// The opt-in bench specs report "skipped" when their large
				// archive is absent (PR body + file comment promise a skip,
				// not a red run) — same convention as DesignFetchBenchmarkTests.
				GTEST_SKIP() << "large-design archive not present (new_designs/) — bench scenario unavailable";
			}

		// Phase 0 — parse-only baseline on THE cache the serve phases reuse
		// (one parse total; destroying a probe cache first would leave glibc
		// arenas resident and poison every later reading). This is also the
		// production steady state: the pod holds the parsed design and serves
		// streams from it.
		auto cache = std::make_shared<DesignCache>(dir.string());
		std::uint64_t parseOnlyPeakKb = 0;
		tryResetPeakRss();
		ASSERT_NE(cache->GetDesign(designName), nullptr) << "parse failed";
		parseOnlyPeakKb = peakRssKb();
		std::cout << "[BENCH] stream design=\"" << designName << "\" phase=parse-only"
			<< " peakRssKb=" << parseOnlyPeakKb
			<< " peakRssGiB=" << (static_cast<double>(parseOnlyPeakKb) / (1024.0 * 1024.0)) << std::endl;

		// The streaming-serve budget: the serve path may add at most ~1 GiB
		// over the parse baseline (gRPC runtime + client + one chunk + the
		// connectivity index + spool write buffering), and the absolute peak
		// must fit the 10 GiB pod with comfortable margin (~6 GiB cap). The
		// pre-M6 monolithic path peaked ~10-12.6 GiB (OOM) on these designs.
		constexpr std::uint64_t kServeMarginalBudgetKb = 1ull * 1024ull * 1024ull;
		constexpr std::uint64_t kAbsoluteBudgetKb = 6ull * 1024ull * 1024ull;

		{
			ServeEnv env;
			MakeServeEnv(env, cache, /*monolithicBudget=*/0 /* real default */);

			// Cold: walk + spool tee + wire (the serve path under test; the
			// design is already parsed on the cache above).
			const auto baselineKb = currentRssKb();
			const auto cold = ConsumeStream(*env.stub, designName);
			const auto coldPeakKb = peakRssKb();
			const auto marginalKb = (coldPeakKb > baselineKb) ? (coldPeakKb - baselineKb) : 0;

			std::cout << "[BENCH] stream design=\"" << designName << "\" phase=cold-serve"
				<< " ok=" << (cold.ok ? "true" : "false")
				<< " chunks=" << cold.chunks
				<< " wire_bytes=" << cold.wireBytes
				<< " ms=" << cold.milliseconds
				<< " peakRssKb=" << coldPeakKb
				<< " peakRssGiB=" << (static_cast<double>(coldPeakKb) / (1024.0 * 1024.0))
				<< " baselineKb=" << baselineKb
				<< " marginalKb=" << marginalKb
				<< " parseOnlyKb=" << parseOnlyPeakKb
				<< std::endl;
			ASSERT_TRUE(cold.ok) << cold.error;
			EXPECT_GT(cold.wireBytes, 1000ull * 1000ull) << "large design stream should be >1 MB";
			EXPECT_LE(coldPeakKb, kAbsoluteBudgetKb)
				<< "cold GetDesignStream absolute peak must fit the 10 GiB pod with margin";
			EXPECT_LE(coldPeakKb, parseOnlyPeakKb + kServeMarginalBudgetKb)
				<< "the streaming serve path must add at most ~1 GiB over the parse-only baseline";

			// Dispatch gate: the monolithic RPC must refuse the design (the
			// pre-M6 OOM path is now a clean error; peak RSS must not move).
			const auto monolithic = FetchMonolithicStatus(*env.stub, designName);
			std::cout << "[BENCH] stream design=\"" << designName << "\" phase=monolithic"
				<< " status=" << monolithic.error_code()
				<< " peakRssKb=" << peakRssKb() << std::endl;
			EXPECT_EQ(monolithic.error_code(), grpc::StatusCode::RESOURCE_EXHAUSTED);
			EXPECT_LE(peakRssKb(), coldPeakKb + 65536)
				<< "the monolithic rejection must not build anything";

			// Warm: re-emitted from the disk spool (no product-model walk).
			// The FIRST warm serve re-reads the spool from disk, so its
			// latency is page-cache-cold while the cold walk above read only
			// memory — comparing those two timings is flaky (observed warm
			// 11.3 s vs cold 7.2 s on one run, 5.0 s vs 7.5 s on the next,
			// with chunk identity holding both times). Serve one discard
			// pass to pull the spool into the page cache, then time the
			// second (hot) warm serve against the cold walk.
			tryResetPeakRss();
			const auto warmUp = ConsumeStream(*env.stub, designName);
			ASSERT_TRUE(warmUp.ok) << warmUp.error;
			EXPECT_EQ(warmUp.chunks, cold.chunks) << "warm serve must replay the same chunk sequence";
			EXPECT_EQ(warmUp.wireBytes, cold.wireBytes);

			const auto warm = ConsumeStream(*env.stub, designName);
			const auto warmPeakKb = peakRssKb();
			std::cout << "[BENCH] stream design=\"" << designName << "\" phase=warm"
				<< " ok=" << (warm.ok ? "true" : "false")
				<< " chunks=" << warm.chunks
				<< " wire_bytes=" << warm.wireBytes
				<< " ms=" << warm.milliseconds
				<< " peakRssKb=" << warmPeakKb
				<< " currentRssKb=" << currentRssKb() << std::endl;
			ASSERT_TRUE(warm.ok) << warm.error;
			EXPECT_EQ(warm.chunks, cold.chunks) << "warm serve must replay the same chunk sequence";
			EXPECT_EQ(warm.wireBytes, cold.wireBytes);
			EXPECT_LE(warm.milliseconds, cold.milliseconds)
				<< "warm (spool, page-cache-hot) serve must not be slower than the cold walk";
			EXPECT_EQ(env.service->streamSpoolCache().WarmOpens(), 2ull)
				<< "discard pass + timed pass are both warm serves";

			// The warm replay is part of the serve path under test, and it
			// measured HIGHER than cold in review (5.05 vs 4.98 GiB on
			// 350-41017) — so its peak obeys the same budgets as the cold
			// serve (it never walks the product model; a breach here means
			// the spool reader is buffering instead of streaming).
			EXPECT_LE(warmPeakKb, kAbsoluteBudgetKb)
				<< "warm GetDesignStream absolute peak must fit the 10 GiB pod with margin";
			EXPECT_LE(warmPeakKb, parseOnlyPeakKb + kServeMarginalBudgetKb)
				<< "the warm spool replay must add at most ~1 GiB over the parse-only baseline";

			std::cout << "[BENCH] stream design=\"" << designName << "\" spoolBytes="
				<< env.service->streamSpoolCache().TotalSpoolBytes() << std::endl;
		} // ServeEnv torn down: cache + spool released
	}

	};
	TEST_F(DesignStreamMemoryTest, LargeDesign_200_40628_StreamWithinRssBudget)
	{
		RunLargeDesignScenario("200-40628_Rev1_v7.tgz", "200-40628_Rev1_v7");
	}

	TEST_F(DesignStreamMemoryTest, LargeDesign_350_41017_StreamWithinRssBudget)
	{
		RunLargeDesignScenario("350-41017_rev1_odbjob_v7.tgz", "350-41017_rev1_odbjob_v7");
	}

	TEST_F(DesignStreamMemoryTest, SampleDesign_ColdStreamVsMonolithic_Perf)
	{
		const std::string designName = "sample_design";

		// Phase 1 — default (monolithic) serve, fresh cache: cold RPC then a
		// warm RPC (in-RAM cached bytes path).
		double coldMonolithicMs = 0.0;
		double warmMonolithicMs = 0.0;
		std::uint64_t monolithicBytes = 0;
		{
			ServeEnv env;
			MakeServeEnv(env, std::make_shared<DesignCache>(m_scratchDir.string()), /*monolithicBudget=*/0);
			tryResetPeakRss();
			auto start = std::chrono::steady_clock::now();
			const auto status = FetchMonolithicStatus(*env.stub, designName, &monolithicBytes);
			coldMonolithicMs = std::chrono::duration<double, std::milli>(
				std::chrono::steady_clock::now() - start).count();
			ASSERT_TRUE(status.ok()) << status.error_message();

			// Give the background pre-serialization a moment to land so the
			// warm fetch takes the cached-bytes path.
			const auto preSerialized = waitForCondition(
				[&env, &designName]() { return env.cache->HasCachedResponse(designName); },
				std::chrono::seconds(30));
			ASSERT_TRUE(preSerialized) << "background pre-serialization did not commit";

			start = std::chrono::steady_clock::now();
			const auto warmStatus = FetchMonolithicStatus(*env.stub, designName);
			warmMonolithicMs = std::chrono::duration<double, std::milli>(
				std::chrono::steady_clock::now() - start).count();
			ASSERT_TRUE(warmStatus.ok()) << warmStatus.error_message();
		}

		// Phase 2 — streaming serve of the same design, fresh cache, budget
		// forced low so the disk-spool path is exercised (sample_design is
		// under the real budget, so no spool would exist otherwise).
		double coldStreamMs = 0.0;
		double warmStreamMs = 0.0;
		{
			ServeEnv env;
			MakeServeEnv(env, std::make_shared<DesignCache>(m_scratchDir.string()), /*monolithicBudget=*/1);
			tryResetPeakRss();

			auto cold = ConsumeStream(*env.stub, designName);
			coldStreamMs = cold.milliseconds;
			ASSERT_TRUE(cold.ok) << cold.error;

			auto warm = ConsumeStream(*env.stub, designName);
			warmStreamMs = warm.milliseconds;
			ASSERT_TRUE(warm.ok) << warm.error;
			EXPECT_EQ(env.service->streamSpoolCache().WarmOpens(), 1ull);
		}

		std::cout << "[BENCH] perf design=\"" << designName << "\""
			<< " monolithic_bytes=" << monolithicBytes
			<< " cold_monolithic_ms=" << coldMonolithicMs
			<< " warm_monolithic_ms=" << warmMonolithicMs
			<< " cold_stream_ms=" << coldStreamMs
			<< " warm_stream_ms=" << warmStreamMs
			<< " cold_ratio=" << (coldMonolithicMs > 0 ? coldStreamMs / coldMonolithicMs : 0.0)
			<< std::endl;

		// Perf gate: cold stream within ~1.5x monolithic; warm <= monolithic.
		EXPECT_LE(coldStreamMs, 1.5 * coldMonolithicMs + 1.0)
			<< "cold stream is >1.5x monolithic";
		EXPECT_LE(warmStreamMs, warmMonolithicMs + 1.0)
			<< "warm spool serve is slower than the monolithic warm path";
	}
}
