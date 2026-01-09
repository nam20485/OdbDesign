#include "GrpcServiceConfig.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <crow/json.h>
#include <filesystem>

namespace OdbDesignServer
{
    namespace Config
    {
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
                        if (compressionSection.has("enabled"))
                        {
                            result.config->compression_enabled = compressionSection["enabled"].b();
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
