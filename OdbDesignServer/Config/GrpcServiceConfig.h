#pragma once

#include <memory>
#include <string>
#include <grpc/impl/compression_types.h>

namespace OdbDesignServer
{
    namespace Config
    {
        struct GrpcServiceConfig
        {
            // Message size limits (in MB)
            int max_receive_message_size_mb = 150;  // Default 150MB (updated to handle large profile files)
            int max_send_message_size_mb = 150;     // Default 150MB (updated to handle large profile files)

            // Compression configuration — abstract quality level (gRPC selects algorithm per peer)
            // "none" = GRPC_COMPRESS_LEVEL_NONE, "low" = LOW, "medium" = MED, "high" = HIGH
            grpc_compression_level compression_level = GRPC_COMPRESS_LEVEL_HIGH;

            // Batch streaming configuration
            bool enable_batch_streaming = true;  // Feature flag for gradual rollout
            int batch_size = 500;                // Features per batch (100-1000)

            // Thread pool configuration
            int max_threads = 8;       // Maximum gRPC server threads (ResourceQuota)
            int min_pollers = 1;       // Minimum polling threads

            // Keepalive configuration (seconds)
            int keepalive_time_s = 30;       // Send keepalive ping every N seconds on idle connections
            int keepalive_timeout_s = 10;    // Close connection if no response within N seconds
            bool keepalive_permit_without_calls = true;  // Send pings even with no active RPCs

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

            // Parse compression level string to gRPC enum
            static grpc_compression_level ParseCompressionLevel(const std::string& value);

            // Convert compression level enum to human-readable string
            static std::string CompressionLevelToString(grpc_compression_level level);
        };
    }
}
