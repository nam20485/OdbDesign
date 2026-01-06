#pragma once

#include <memory>
#include <string>

namespace OdbDesignServer
{
    namespace Config
    {
        struct GrpcServiceConfig
        {
            // Batch streaming configuration
            bool enable_batch_streaming = true;  // Feature flag for gradual rollout
            int batch_size = 500;                // Features per batch (100-1000)

            // Clamp batch_size to valid range [100, 1000]
            void ClampBatchSize()
            {
                if (batch_size < 100)
                    batch_size = 100;
                else if (batch_size > 1000)
                    batch_size = 1000;
            }

            // Create default configuration
            static std::shared_ptr<GrpcServiceConfig> CreateDefault()
            {
                return std::make_shared<GrpcServiceConfig>();
            }

            // Load configuration from JSON file
            // Returns default config if file doesn't exist or parsing fails
            static std::shared_ptr<GrpcServiceConfig> LoadFromFile(const std::string& configPath);
        };
    }
}
