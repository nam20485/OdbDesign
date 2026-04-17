#include <gtest/gtest.h>
#include <grpcpp/server_context.h>
#include <memory>

#include "Fixtures/FileArchiveLoadFixture.h"
#include "OdbDesignServer/Services/OdbDesignServiceImpl.h"
#include "OdbDesignServer/Config/GrpcServiceConfig.h"
#include "service.pb.h"

using namespace Odb::Test::Fixtures;
using namespace OdbDesignServer::Services;
using namespace OdbDesignServer::Config;
using namespace Odb::Grpc;

// Note: ServerWriter is final, so we can't mock it for unit tests
// Detailed batching logic tests should be done via integration tests
// These unit tests focus on error handling and configuration

class GrpcBatchStreamFixture : public FileArchiveLoadFixture
{
protected:
    void SetUp() override
    {
        FileArchiveLoadFixture::SetUp();
        // Create default config
        m_config = GrpcServiceConfig::CreateDefault();
        // Convert unique_ptr to shared_ptr for the service constructor
        // Assign to member first for exception safety
        m_sharedDesignCache = std::shared_ptr<Odb::Lib::App::DesignCache>(m_pDesignCache.release());
        m_service = std::make_unique<OdbDesignServiceImpl>(m_sharedDesignCache, m_config);
    }

    std::shared_ptr<Odb::Lib::App::DesignCache> m_sharedDesignCache;
    std::shared_ptr<GrpcServiceConfig> m_config;
    std::unique_ptr<OdbDesignServiceImpl> m_service;
};


// Test feature flag - returns UNIMPLEMENTED when disabled
TEST_F(GrpcBatchStreamFixture, ReturnsUnimplementedWhenFeatureFlagDisabled)
{
    grpc::ServerContext ctx;
    GetLayerFeaturesRequest req;
    
    req.set_design_name("designodb_rigidflex");
    req.set_step_name("cellular_flip-phone");
    req.set_layer_name("signal_2");

    // Disable batch streaming
    m_config->enable_batch_streaming = false;
    m_service = std::make_unique<OdbDesignServiceImpl>(m_sharedDesignCache, m_config);

    // Use nullptr - method returns UNIMPLEMENTED before using writer
    auto status = m_service->GetLayerFeaturesBatchStream(&ctx, &req, nullptr);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::UNIMPLEMENTED);
}

// Test NOT_FOUND for missing design
TEST_F(GrpcBatchStreamFixture, ReturnsNotFoundForMissingDesign)
{
    grpc::ServerContext ctx;
    GetLayerFeaturesRequest req;
    
    req.set_design_name("does_not_exist");
    req.set_step_name("any");
    req.set_layer_name("any");

    // Use nullptr - method returns NOT_FOUND before using writer
    auto status = m_service->GetLayerFeaturesBatchStream(&ctx, &req, nullptr);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::NOT_FOUND);
}

// Test NOT_FOUND for missing step
TEST_F(GrpcBatchStreamFixture, ReturnsNotFoundForMissingStep)
{
    grpc::ServerContext ctx;
    GetLayerFeaturesRequest req;
    
    req.set_design_name("designodb_rigidflex");
    req.set_step_name("does_not_exist");
    req.set_layer_name("signal_2");

    // Use nullptr - method returns NOT_FOUND before using writer
    auto status = m_service->GetLayerFeaturesBatchStream(&ctx, &req, nullptr);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::NOT_FOUND);
}

// Test NOT_FOUND for missing layer
TEST_F(GrpcBatchStreamFixture, ReturnsNotFoundForMissingLayer)
{
    grpc::ServerContext ctx;
    GetLayerFeaturesRequest req;
    
    req.set_design_name("designodb_rigidflex");
    req.set_step_name("cellular_flip-phone");
    req.set_layer_name("does_not_exist");

    // Use nullptr - method returns NOT_FOUND before using writer
    auto status = m_service->GetLayerFeaturesBatchStream(&ctx, &req, nullptr);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::NOT_FOUND);
}

// Test batch size clamping - enforce 100-1000 range
TEST_F(GrpcBatchStreamFixture, ClampsBatchSizeToValidRange)
{
    auto config = std::make_shared<GrpcServiceConfig>();
    
    // Test clamping below minimum
    config->batch_size = 50;
    config->ClampBatchSize();
    EXPECT_EQ(config->batch_size, 100);

    // Test clamping above maximum
    config->batch_size = 2000;
    config->ClampBatchSize();
    EXPECT_EQ(config->batch_size, 1000);

    // Test valid value unchanged
    config->batch_size = 500;
    config->ClampBatchSize();
    EXPECT_EQ(config->batch_size, 500);
}
