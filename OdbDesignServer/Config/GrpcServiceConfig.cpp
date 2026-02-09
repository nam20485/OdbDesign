#include "GrpcServiceConfig.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <crow/json.h>
#include <filesystem>
#include <cctype>

namespace OdbDesignServer
{
    namespace Config
    {
        grpc_compression_level GrpcServiceConfig::ParseCompressionLevel(const std::string& value)
        {
            // Case-insensitive comparison
            std::string lower = value;
            std::transform(lower.begin(), lower.end(), lower.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

            if (lower == "none")   return GRPC_COMPRESS_LEVEL_NONE;
            if (lower == "low")    return GRPC_COMPRESS_LEVEL_LOW;
            if (lower == "medium") return GRPC_COMPRESS_LEVEL_MED;
            if (lower == "high")   return GRPC_COMPRESS_LEVEL_HIGH;
            // Unknown value → default to high (matches current gzip behavior)
            return GRPC_COMPRESS_LEVEL_HIGH;
        }

        std::string GrpcServiceConfig::CompressionLevelToString(grpc_compression_level level)
        {
            switch (level)
            {
                case GRPC_COMPRESS_LEVEL_NONE: return "none";
                case GRPC_COMPRESS_LEVEL_LOW:  return "low";
                case GRPC_COMPRESS_LEVEL_MED:  return "medium";
                case GRPC_COMPRESS_LEVEL_HIGH: return "high";
                default: return "unknown";
            }
        }
        GrpcServiceConfig::LoadResult GrpcServiceConfig::LoadFromFile(const std::string& configPath)
        {
            LoadResult result;
            result.config = std::make_shared<GrpcServiceConfig>();

            // Check if file exists
            if (!std::filesystem::exists(configPath))
            {
                result.loadedFromFile = false;
                result.message = "Using default gRPC config (" + configPath + " not found)";
                return result;
            }

            try
            {
                // Read file contents
                std::ifstream file(configPath);
                if (!file.is_open())
                {
                    result.loadedFromFile = false;
                    result.message = "Using default gRPC config (could not open " + configPath + ")";
                    return result;
                }

                std::stringstream buffer;
                buffer << file.rdbuf();
                file.close();

                std::string jsonContent = buffer.str();
                if (jsonContent.empty())
                {
                    result.loadedFromFile = false;
                    result.message = "Using default gRPC config (" + configPath + " is empty)";
                    return result;
                }

                // Parse JSON using Crow
                auto json = crow::json::load(jsonContent);
                if (!json)
                {
                    result.loadedFromFile = false;
                    result.message = "Using default gRPC config (failed to parse JSON in " + configPath + ")";
                    return result;
                }

                // Extract grpc configuration
                if (json.has("grpc"))
                {
                    auto grpcSection = json["grpc"];

                    // Extract message size limits
                    if (grpcSection.has("max_receive_message_size_mb"))
                    {
                        int value = static_cast<int>(grpcSection["max_receive_message_size_mb"].i());
                        // Clamp to reasonable range: 1MB minimum, 1000MB maximum
                        result.config->max_receive_message_size_mb = std::max(1, std::min(1000, value));
                    }

                    if (grpcSection.has("max_send_message_size_mb"))
                    {
                        int value = static_cast<int>(grpcSection["max_send_message_size_mb"].i());
                        // Clamp to reasonable range: 1MB minimum, 1000MB maximum
                        result.config->max_send_message_size_mb = std::max(1, std::min(1000, value));
                    }

                    // Extract compression configuration
                    if (grpcSection.has("compression"))
                    {
                        auto compressionSection = grpcSection["compression"];
                        if (compressionSection.has("level"))
                        {
                            result.config->compression_level = ParseCompressionLevel(
                                static_cast<std::string>(compressionSection["level"].s()));
                        }
                        // Backward compatibility: support legacy "enabled" boolean
                        // If "level" is present, it takes precedence
                        else if (compressionSection.has("enabled"))
                        {
                            result.config->compression_level = compressionSection["enabled"].b()
                                ? GRPC_COMPRESS_LEVEL_HIGH
                                : GRPC_COMPRESS_LEVEL_NONE;
                        }
                    }

                    // Extract batch streaming configuration
                    if (grpcSection.has("batch_streaming"))
                    {
                        auto batchSection = grpcSection["batch_streaming"];

                        if (batchSection.has("enabled"))
                        {
                            result.config->enable_batch_streaming = batchSection["enabled"].b();
                        }

                        if (batchSection.has("batch_size"))
                        {
                            result.config->batch_size = static_cast<int>(batchSection["batch_size"].i());
                            result.config->ClampBatchSize(); // Ensure valid range
                        }
                    }

                    // Extract thread pool configuration
                    if (grpcSection.has("thread_pool"))
                    {
                        auto threadSection = grpcSection["thread_pool"];

                        if (threadSection.has("max_threads"))
                        {
                            int value = static_cast<int>(threadSection["max_threads"].i());
                            result.config->max_threads = std::max(1, std::min(64, value));
                        }

                        if (threadSection.has("min_pollers"))
                        {
                            int value = static_cast<int>(threadSection["min_pollers"].i());
                            result.config->min_pollers = std::max(1, std::min(16, value));
                        }
                    }

                    // Extract keepalive configuration
                    if (grpcSection.has("keepalive"))
                    {
                        auto keepaliveSection = grpcSection["keepalive"];

                        if (keepaliveSection.has("time_s"))
                        {
                            int value = static_cast<int>(keepaliveSection["time_s"].i());
                            result.config->keepalive_time_s = std::max(1, std::min(3600, value));
                        }

                        if (keepaliveSection.has("timeout_s"))
                        {
                            int value = static_cast<int>(keepaliveSection["timeout_s"].i());
                            result.config->keepalive_timeout_s = std::max(1, std::min(300, value));
                        }

                        if (keepaliveSection.has("permit_without_calls"))
                        {
                            result.config->keepalive_permit_without_calls = keepaliveSection["permit_without_calls"].b();
                        }
                    }
                }

                result.loadedFromFile = true;
                result.message = "Loaded gRPC config from: " + configPath;
            }
            catch (const std::exception& e)
            {
                // TODO: Refactor config error handling - preserve partially loaded config instead of silently falling back to defaults
                // Issue: When JSON parsing fails, any successfully parsed values are lost
                // Proposed: Store config values as parsed, only reset fields that failed, or fail explicitly
                // Log exception and return default config
                result.config = std::make_shared<GrpcServiceConfig>();
                result.loadedFromFile = false;
                result.message = "Using default gRPC config (exception loading " + configPath + ": " + e.what() + ")";
            }
            catch (...)
            {
                // TODO: Refactor config error handling - preserve partially loaded config instead of silently falling back to defaults
                // Issue: When JSON parsing fails, any successfully parsed values are lost
                // Proposed: Store config values as parsed, only reset fields that failed, or fail explicitly
                // Log unknown exception and return default config
                result.config = std::make_shared<GrpcServiceConfig>();
                result.loadedFromFile = false;
                result.message = "Using default gRPC config (unknown exception loading " + configPath + ")";
            }

            return result;
        }
    }
}
