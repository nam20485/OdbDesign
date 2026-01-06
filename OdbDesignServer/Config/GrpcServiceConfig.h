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

            // Load result containing config and status
            struct LoadResult
            {
                std::shared_ptr<GrpcServiceConfig> config;
                bool loadedFromFile = false;  // true if loaded from file, false if using defaults
                std::string message;          // Status message for logging
            };

            // Load configuration from JSON file
            // Returns LoadResult containing config and whether it was loaded from file
            static LoadResult LoadFromFile(const std::string& configPath);
        };
    }
}
