#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Fixtures/TestUtils.h"
#include "OdbDesignServer/Config/GrpcServiceConfig.h"
#include <fstream>
#include <filesystem>
#include <string>
#include <memory>
#include <grpc/impl/compression_types.h>

using namespace OdbDesignServer::Config;
using namespace Odb::Test::Utils;
using namespace testing;

namespace Odb::Test::Config
{

    // =========================================================================
    // GrpcServiceConfig Tests
    // =========================================================================
    // Tests for Phase 1 config additions: thread_pool (max_threads, min_pollers),
    // keepalive (time_s, timeout_s, permit_without_calls), and value clamping.
    // =========================================================================

    class GrpcServiceConfigTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            m_tempDir = TestUtils::createManagedTempDirectory("grpc_config_test");
        }

        void TearDown() override
        {
            m_tempDir.reset();
        }

        // Helper: write JSON to a temp file and return its path
        std::string writeConfigFile(const std::string& json, const std::string& filename = "config.json")
        {
            auto path = m_tempDir->path() / filename;
            std::ofstream f(path);
            f << json;
            f.close();
            return path.string();
        }

        std::unique_ptr<TestUtils::TempResource> m_tempDir;
    };

    // ---- Defaults ----

    TEST_F(GrpcServiceConfigTest, CreateDefault_HasExpectedValues)
    {
        auto config = GrpcServiceConfig::CreateDefault();
        ASSERT_NE(config, nullptr);

        // Pre-existing defaults
        EXPECT_EQ(config->max_receive_message_size_mb, 150);
        EXPECT_EQ(config->max_send_message_size_mb, 150);
        EXPECT_EQ(config->compression_level, GRPC_COMPRESS_LEVEL_HIGH);
        EXPECT_TRUE(config->enable_batch_streaming);
        EXPECT_EQ(config->batch_size, 500);

        // Phase 1 additions
        EXPECT_EQ(config->max_threads, 8);
        EXPECT_EQ(config->min_pollers, 1);
        EXPECT_EQ(config->keepalive_time_s, 30);
        EXPECT_EQ(config->keepalive_timeout_s, 10);
        EXPECT_TRUE(config->keepalive_permit_without_calls);
    }

    // ---- File not found: graceful fallback ----

    TEST_F(GrpcServiceConfigTest, LoadFromFile_FileNotFound_ReturnsDefaults)
    {
        auto result = GrpcServiceConfig::LoadFromFile("/nonexistent/path/config.json");

        EXPECT_FALSE(result.loadedFromFile);
        ASSERT_NE(result.config, nullptr);

        // Should match defaults
        EXPECT_EQ(result.config->max_threads, 8);
        EXPECT_EQ(result.config->keepalive_time_s, 30);
    }

    TEST_F(GrpcServiceConfigTest, LoadFromFile_EmptyFile_ReturnsDefaults)
    {
        auto path = writeConfigFile("");
        auto result = GrpcServiceConfig::LoadFromFile(path);

        EXPECT_FALSE(result.loadedFromFile);
        ASSERT_NE(result.config, nullptr);
        EXPECT_EQ(result.config->max_threads, 8);
    }

    TEST_F(GrpcServiceConfigTest, LoadFromFile_InvalidJson_ReturnsDefaults)
    {
        auto path = writeConfigFile("not valid json {{{");
        auto result = GrpcServiceConfig::LoadFromFile(path);

        EXPECT_FALSE(result.loadedFromFile);
        ASSERT_NE(result.config, nullptr);
        EXPECT_EQ(result.config->max_threads, 8);
    }

    // ---- Thread pool config loading ----

    TEST_F(GrpcServiceConfigTest, LoadFromFile_ThreadPool_ParsesCorrectly)
    {
        auto path = writeConfigFile(R"({
            "grpc": {
                "thread_pool": {
                    "max_threads": 16,
                    "min_pollers": 4
                }
            }
        })");

        auto result = GrpcServiceConfig::LoadFromFile(path);
        EXPECT_TRUE(result.loadedFromFile);
        ASSERT_NE(result.config, nullptr);

        EXPECT_EQ(result.config->max_threads, 16);
        EXPECT_EQ(result.config->min_pollers, 4);
    }

    TEST_F(GrpcServiceConfigTest, LoadFromFile_ThreadPool_ClampsMaxThreads)
    {
        // max_threads below minimum → clamped to 1
        auto path = writeConfigFile(R"({
            "grpc": {
                "thread_pool": {
                    "max_threads": 0
                }
            }
        })");

        auto result = GrpcServiceConfig::LoadFromFile(path);
        EXPECT_TRUE(result.loadedFromFile);
        EXPECT_EQ(result.config->max_threads, 1);
    }

    TEST_F(GrpcServiceConfigTest, LoadFromFile_ThreadPool_ClampsMaxThreadsHigh)
    {
        // max_threads above maximum → clamped to 64
        auto path = writeConfigFile(R"({
            "grpc": {
                "thread_pool": {
                    "max_threads": 999
                }
            }
        })");

        auto result = GrpcServiceConfig::LoadFromFile(path);
        EXPECT_TRUE(result.loadedFromFile);
        EXPECT_EQ(result.config->max_threads, 64);
    }

    TEST_F(GrpcServiceConfigTest, LoadFromFile_ThreadPool_ClampsMinPollersHigh)
    {
        // min_pollers above maximum → clamped to 16
        auto path = writeConfigFile(R"({
            "grpc": {
                "thread_pool": {
                    "min_pollers": 100
                }
            }
        })");

        auto result = GrpcServiceConfig::LoadFromFile(path);
        EXPECT_TRUE(result.loadedFromFile);
        EXPECT_EQ(result.config->min_pollers, 16);
    }

    // ---- Keepalive config loading ----

    TEST_F(GrpcServiceConfigTest, LoadFromFile_Keepalive_ParsesCorrectly)
    {
        auto path = writeConfigFile(R"({
            "grpc": {
                "keepalive": {
                    "time_s": 60,
                    "timeout_s": 20,
                    "permit_without_calls": false
                }
            }
        })");

        auto result = GrpcServiceConfig::LoadFromFile(path);
        EXPECT_TRUE(result.loadedFromFile);
        ASSERT_NE(result.config, nullptr);

        EXPECT_EQ(result.config->keepalive_time_s, 60);
        EXPECT_EQ(result.config->keepalive_timeout_s, 20);
        EXPECT_FALSE(result.config->keepalive_permit_without_calls);
    }

    TEST_F(GrpcServiceConfigTest, LoadFromFile_Keepalive_ClampsTimeLow)
    {
        auto path = writeConfigFile(R"({
            "grpc": {
                "keepalive": {
                    "time_s": 0
                }
            }
        })");

        auto result = GrpcServiceConfig::LoadFromFile(path);
        EXPECT_TRUE(result.loadedFromFile);
        EXPECT_EQ(result.config->keepalive_time_s, 1);
    }

    TEST_F(GrpcServiceConfigTest, LoadFromFile_Keepalive_ClampsTimeHigh)
    {
        auto path = writeConfigFile(R"({
            "grpc": {
                "keepalive": {
                    "time_s": 99999
                }
            }
        })");

        auto result = GrpcServiceConfig::LoadFromFile(path);
        EXPECT_TRUE(result.loadedFromFile);
        EXPECT_EQ(result.config->keepalive_time_s, 3600);
    }

    TEST_F(GrpcServiceConfigTest, LoadFromFile_Keepalive_ClampsTimeoutLow)
    {
        auto path = writeConfigFile(R"({
            "grpc": {
                "keepalive": {
                    "timeout_s": 0
                }
            }
        })");

        auto result = GrpcServiceConfig::LoadFromFile(path);
        EXPECT_TRUE(result.loadedFromFile);
        EXPECT_EQ(result.config->keepalive_timeout_s, 1);
    }

    TEST_F(GrpcServiceConfigTest, LoadFromFile_Keepalive_ClampsTimeoutHigh)
    {
        auto path = writeConfigFile(R"({
            "grpc": {
                "keepalive": {
                    "timeout_s": 999
                }
            }
        })");

        auto result = GrpcServiceConfig::LoadFromFile(path);
        EXPECT_TRUE(result.loadedFromFile);
        EXPECT_EQ(result.config->keepalive_timeout_s, 300);
    }

    // ---- Full config loading (all sections) ----

    TEST_F(GrpcServiceConfigTest, LoadFromFile_FullConfig_ParsesAllSections)
    {
        auto path = writeConfigFile(R"({
            "grpc": {
                "max_receive_message_size_mb": 200,
                "max_send_message_size_mb": 200,
                "compression": {
                    "level": "low"
                },
                "batch_streaming": {
                    "enabled": false,
                    "batch_size": 250
                },
                "thread_pool": {
                    "max_threads": 12,
                    "min_pollers": 2
                },
                "keepalive": {
                    "time_s": 45,
                    "timeout_s": 15,
                    "permit_without_calls": true
                }
            }
        })");

        auto result = GrpcServiceConfig::LoadFromFile(path);
        EXPECT_TRUE(result.loadedFromFile);
        ASSERT_NE(result.config, nullptr);

        // Pre-existing fields
        EXPECT_EQ(result.config->max_receive_message_size_mb, 200);
        EXPECT_EQ(result.config->max_send_message_size_mb, 200);
        EXPECT_EQ(result.config->compression_level, GRPC_COMPRESS_LEVEL_LOW);
        EXPECT_FALSE(result.config->enable_batch_streaming);
        EXPECT_EQ(result.config->batch_size, 250);

        // Phase 1 additions
        EXPECT_EQ(result.config->max_threads, 12);
        EXPECT_EQ(result.config->min_pollers, 2);
        EXPECT_EQ(result.config->keepalive_time_s, 45);
        EXPECT_EQ(result.config->keepalive_timeout_s, 15);
        EXPECT_TRUE(result.config->keepalive_permit_without_calls);
    }

    // ---- Partial config: missing sections default correctly ----

    TEST_F(GrpcServiceConfigTest, LoadFromFile_NoThreadPoolSection_KeepsDefaults)
    {
        auto path = writeConfigFile(R"({
            "grpc": {
                "keepalive": {
                    "time_s": 60
                }
            }
        })");

        auto result = GrpcServiceConfig::LoadFromFile(path);
        EXPECT_TRUE(result.loadedFromFile);
        EXPECT_EQ(result.config->max_threads, 8);   // default
        EXPECT_EQ(result.config->min_pollers, 1);    // default
        EXPECT_EQ(result.config->keepalive_time_s, 60);  // loaded
    }

    TEST_F(GrpcServiceConfigTest, LoadFromFile_NoKeepaliveSection_KeepsDefaults)
    {
        auto path = writeConfigFile(R"({
            "grpc": {
                "thread_pool": {
                    "max_threads": 4
                }
            }
        })");

        auto result = GrpcServiceConfig::LoadFromFile(path);
        EXPECT_TRUE(result.loadedFromFile);
        EXPECT_EQ(result.config->max_threads, 4);           // loaded
        EXPECT_EQ(result.config->keepalive_time_s, 30);     // default
        EXPECT_EQ(result.config->keepalive_timeout_s, 10);  // default
        EXPECT_TRUE(result.config->keepalive_permit_without_calls);  // default
    }

    TEST_F(GrpcServiceConfigTest, LoadFromFile_EmptyGrpcSection_KeepsAllDefaults)
    {
        auto path = writeConfigFile(R"({
            "grpc": {}
        })");

        auto result = GrpcServiceConfig::LoadFromFile(path);
        EXPECT_TRUE(result.loadedFromFile);
        auto defaults = GrpcServiceConfig::CreateDefault();

        EXPECT_EQ(result.config->max_threads, defaults->max_threads);
        EXPECT_EQ(result.config->min_pollers, defaults->min_pollers);
        EXPECT_EQ(result.config->keepalive_time_s, defaults->keepalive_time_s);
        EXPECT_EQ(result.config->keepalive_timeout_s, defaults->keepalive_timeout_s);
        EXPECT_EQ(result.config->keepalive_permit_without_calls, defaults->keepalive_permit_without_calls);
    }

    // ---- Boundary values ----

    TEST_F(GrpcServiceConfigTest, LoadFromFile_BoundaryValues_ClampedCorrectly)
    {
        // Test exact boundary values
        auto path = writeConfigFile(R"({
            "grpc": {
                "thread_pool": {
                    "max_threads": 1,
                    "min_pollers": 1
                },
                "keepalive": {
                    "time_s": 1,
                    "timeout_s": 1
                }
            }
        })");

        auto result = GrpcServiceConfig::LoadFromFile(path);
        EXPECT_TRUE(result.loadedFromFile);
        EXPECT_EQ(result.config->max_threads, 1);
        EXPECT_EQ(result.config->min_pollers, 1);
        EXPECT_EQ(result.config->keepalive_time_s, 1);
        EXPECT_EQ(result.config->keepalive_timeout_s, 1);
    }

    TEST_F(GrpcServiceConfigTest, LoadFromFile_MaxBoundaryValues_ClampedCorrectly)
    {
        auto path = writeConfigFile(R"({
            "grpc": {
                "thread_pool": {
                    "max_threads": 64,
                    "min_pollers": 16
                },
                "keepalive": {
                    "time_s": 3600,
                    "timeout_s": 300
                }
            }
        })");

        auto result = GrpcServiceConfig::LoadFromFile(path);
        EXPECT_TRUE(result.loadedFromFile);
        EXPECT_EQ(result.config->max_threads, 64);
        EXPECT_EQ(result.config->min_pollers, 16);
        EXPECT_EQ(result.config->keepalive_time_s, 3600);
        EXPECT_EQ(result.config->keepalive_timeout_s, 300);
    }

    // ---- Negative values ----

    TEST_F(GrpcServiceConfigTest, LoadFromFile_NegativeValues_ClampedToMinimum)
    {
        auto path = writeConfigFile(R"({
            "grpc": {
                "thread_pool": {
                    "max_threads": -5,
                    "min_pollers": -1
                },
                "keepalive": {
                    "time_s": -10,
                    "timeout_s": -3
                }
            }
        })");

        auto result = GrpcServiceConfig::LoadFromFile(path);
        EXPECT_TRUE(result.loadedFromFile);
        EXPECT_EQ(result.config->max_threads, 1);
        EXPECT_EQ(result.config->min_pollers, 1);
        EXPECT_EQ(result.config->keepalive_time_s, 1);
        EXPECT_EQ(result.config->keepalive_timeout_s, 1);
    }

    // ---- ClampBatchSize ----

    TEST_F(GrpcServiceConfigTest, ClampBatchSize_BelowMinimum)
    {
        auto config = GrpcServiceConfig::CreateDefault();
        config->batch_size = 50;
        config->ClampBatchSize();
        EXPECT_EQ(config->batch_size, 100);
    }

    TEST_F(GrpcServiceConfigTest, ClampBatchSize_AboveMaximum)
    {
        auto config = GrpcServiceConfig::CreateDefault();
        config->batch_size = 2000;
        config->ClampBatchSize();
        EXPECT_EQ(config->batch_size, 1000);
    }

    TEST_F(GrpcServiceConfigTest, ClampBatchSize_InRange)
    {
        auto config = GrpcServiceConfig::CreateDefault();
        config->batch_size = 500;
        config->ClampBatchSize();
        EXPECT_EQ(config->batch_size, 500);
    }

    // ---- LoadResult message field ----

    TEST_F(GrpcServiceConfigTest, LoadFromFile_Success_MessageContainsPath)
    {
        auto path = writeConfigFile(R"({"grpc": {}})");
        auto result = GrpcServiceConfig::LoadFromFile(path);
        EXPECT_TRUE(result.loadedFromFile);
        EXPECT_THAT(result.message, HasSubstr(path));
    }

    TEST_F(GrpcServiceConfigTest, LoadFromFile_NotFound_MessageContainsPath)
    {
        auto result = GrpcServiceConfig::LoadFromFile("/fake/path/config.json");
        EXPECT_FALSE(result.loadedFromFile);
        EXPECT_THAT(result.message, HasSubstr("/fake/path/config.json"));
    }

    // ---- Compression level parsing ----

    TEST_F(GrpcServiceConfigTest, ParseCompressionLevel_None)
    {
        EXPECT_EQ(GrpcServiceConfig::ParseCompressionLevel("none"), GRPC_COMPRESS_LEVEL_NONE);
        EXPECT_EQ(GrpcServiceConfig::ParseCompressionLevel("NONE"), GRPC_COMPRESS_LEVEL_NONE);
        EXPECT_EQ(GrpcServiceConfig::ParseCompressionLevel("None"), GRPC_COMPRESS_LEVEL_NONE);
    }

    TEST_F(GrpcServiceConfigTest, ParseCompressionLevel_Low)
    {
        EXPECT_EQ(GrpcServiceConfig::ParseCompressionLevel("low"), GRPC_COMPRESS_LEVEL_LOW);
        EXPECT_EQ(GrpcServiceConfig::ParseCompressionLevel("LOW"), GRPC_COMPRESS_LEVEL_LOW);
    }

    TEST_F(GrpcServiceConfigTest, ParseCompressionLevel_Medium)
    {
        EXPECT_EQ(GrpcServiceConfig::ParseCompressionLevel("medium"), GRPC_COMPRESS_LEVEL_MED);
        EXPECT_EQ(GrpcServiceConfig::ParseCompressionLevel("MEDIUM"), GRPC_COMPRESS_LEVEL_MED);
    }

    TEST_F(GrpcServiceConfigTest, ParseCompressionLevel_High)
    {
        EXPECT_EQ(GrpcServiceConfig::ParseCompressionLevel("high"), GRPC_COMPRESS_LEVEL_HIGH);
        EXPECT_EQ(GrpcServiceConfig::ParseCompressionLevel("HIGH"), GRPC_COMPRESS_LEVEL_HIGH);
    }

    TEST_F(GrpcServiceConfigTest, ParseCompressionLevel_UnknownFallsBackToHigh)
    {
        EXPECT_EQ(GrpcServiceConfig::ParseCompressionLevel("unknown"), GRPC_COMPRESS_LEVEL_HIGH);
        EXPECT_EQ(GrpcServiceConfig::ParseCompressionLevel("gzip"), GRPC_COMPRESS_LEVEL_HIGH);
        EXPECT_EQ(GrpcServiceConfig::ParseCompressionLevel(""), GRPC_COMPRESS_LEVEL_HIGH);
    }

    TEST_F(GrpcServiceConfigTest, CompressionLevelToString_AllValues)
    {
        EXPECT_EQ(GrpcServiceConfig::CompressionLevelToString(GRPC_COMPRESS_LEVEL_NONE), "none");
        EXPECT_EQ(GrpcServiceConfig::CompressionLevelToString(GRPC_COMPRESS_LEVEL_LOW), "low");
        EXPECT_EQ(GrpcServiceConfig::CompressionLevelToString(GRPC_COMPRESS_LEVEL_MED), "medium");
        EXPECT_EQ(GrpcServiceConfig::CompressionLevelToString(GRPC_COMPRESS_LEVEL_HIGH), "high");
    }

    TEST_F(GrpcServiceConfigTest, LoadFromFile_CompressionLevel_ParsesAllLevels)
    {
        // Test "none"
        {
            auto path = writeConfigFile(R"({"grpc": {"compression": {"level": "none"}}})");
            auto result = GrpcServiceConfig::LoadFromFile(path);
            EXPECT_TRUE(result.loadedFromFile);
            EXPECT_EQ(result.config->compression_level, GRPC_COMPRESS_LEVEL_NONE);
        }
        // Test "low"
        {
            auto path = writeConfigFile(R"({"grpc": {"compression": {"level": "low"}}})", "config_low.json");
            auto result = GrpcServiceConfig::LoadFromFile(path);
            EXPECT_TRUE(result.loadedFromFile);
            EXPECT_EQ(result.config->compression_level, GRPC_COMPRESS_LEVEL_LOW);
        }
        // Test "medium"
        {
            auto path = writeConfigFile(R"({"grpc": {"compression": {"level": "medium"}}})", "config_med.json");
            auto result = GrpcServiceConfig::LoadFromFile(path);
            EXPECT_TRUE(result.loadedFromFile);
            EXPECT_EQ(result.config->compression_level, GRPC_COMPRESS_LEVEL_MED);
        }
        // Test "high"
        {
            auto path = writeConfigFile(R"({"grpc": {"compression": {"level": "high"}}})", "config_high.json");
            auto result = GrpcServiceConfig::LoadFromFile(path);
            EXPECT_TRUE(result.loadedFromFile);
            EXPECT_EQ(result.config->compression_level, GRPC_COMPRESS_LEVEL_HIGH);
        }
    }

    TEST_F(GrpcServiceConfigTest, LoadFromFile_CompressionLevel_UnknownValueDefaultsToHigh)
    {
        auto path = writeConfigFile(R"({"grpc": {"compression": {"level": "turbo"}}})");
        auto result = GrpcServiceConfig::LoadFromFile(path);
        EXPECT_TRUE(result.loadedFromFile);
        EXPECT_EQ(result.config->compression_level, GRPC_COMPRESS_LEVEL_HIGH);
    }

    TEST_F(GrpcServiceConfigTest, LoadFromFile_CompressionLegacyEnabled_BackwardCompatible)
    {
        // Legacy "enabled: true" should map to HIGH
        {
            auto path = writeConfigFile(R"({"grpc": {"compression": {"enabled": true}}})");
            auto result = GrpcServiceConfig::LoadFromFile(path);
            EXPECT_TRUE(result.loadedFromFile);
            EXPECT_EQ(result.config->compression_level, GRPC_COMPRESS_LEVEL_HIGH);
        }
        // Legacy "enabled: false" should map to NONE
        {
            auto path = writeConfigFile(R"({"grpc": {"compression": {"enabled": false}}})", "config_disabled.json");
            auto result = GrpcServiceConfig::LoadFromFile(path);
            EXPECT_TRUE(result.loadedFromFile);
            EXPECT_EQ(result.config->compression_level, GRPC_COMPRESS_LEVEL_NONE);
        }
    }

    TEST_F(GrpcServiceConfigTest, LoadFromFile_CompressionLevelTakesPrecedenceOverEnabled)
    {
        // When both "level" and "enabled" are present, "level" wins
        auto path = writeConfigFile(R"({"grpc": {"compression": {"level": "low", "enabled": false}}})");
        auto result = GrpcServiceConfig::LoadFromFile(path);
        EXPECT_TRUE(result.loadedFromFile);
        EXPECT_EQ(result.config->compression_level, GRPC_COMPRESS_LEVEL_LOW);
    }

} // namespace Odb::Test::Config
