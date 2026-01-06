#include "GrpcServiceConfig.h"
#include <fstream>
#include <sstream>
#include <crow/json.h>
#include <filesystem>

namespace OdbDesignServer
{
    namespace Config
    {
        std::shared_ptr<GrpcServiceConfig> GrpcServiceConfig::LoadFromFile(const std::string& configPath)
        {
            auto config = std::make_shared<GrpcServiceConfig>();

            // Check if file exists
            if (!std::filesystem::exists(configPath))
            {
                // Return default config if file doesn't exist
                return config;
            }

            try
            {
                // Read file contents
                std::ifstream file(configPath);
                if (!file.is_open())
                {
                    return config; // Return default if can't open
                }

                std::stringstream buffer;
                buffer << file.rdbuf();
                file.close();

                std::string jsonContent = buffer.str();
                if (jsonContent.empty())
                {
                    return config; // Return default if empty
                }

                // Parse JSON using Crow
                auto json = crow::json::load(jsonContent);
                if (!json)
                {
                    return config; // Return default if parsing fails
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
                            config->enable_batch_streaming = batchSection["enabled"].b();
                        }

                        if (batchSection.has("batch_size"))
                        {
                            config->batch_size = static_cast<int>(batchSection["batch_size"].i());
                            config->ClampBatchSize(); // Ensure valid range
                        }
                    }
                }
            }
            catch (...)
            {
                // On any exception, return default config
                return std::make_shared<GrpcServiceConfig>();
            }

            return config;
        }
    }
}
