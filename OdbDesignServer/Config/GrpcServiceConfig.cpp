#include "GrpcServiceConfig.h"
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

                // Extract grpc.batch_streaming configuration
                if (json.has("grpc"))
                {
                    auto grpcSection = json["grpc"];
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
                // Log exception and return default config
                result.config = std::make_shared<GrpcServiceConfig>();
                result.loadedFromFile = false;
                result.message = "Using default gRPC config (exception loading " + configPath + ": " + e.what() + ")";
            }
            catch (...)
            {
                // Log unknown exception and return default config
                result.config = std::make_shared<GrpcServiceConfig>();
                result.loadedFromFile = false;
                result.message = "Using default gRPC config (unknown exception loading " + configPath + ")";
            }

            return result;
        }
    }
}
