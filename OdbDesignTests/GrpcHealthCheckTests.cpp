#include <gtest/gtest.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include <memory>

#include "Fixtures/FileArchiveLoadFixture.h"
#include "OdbDesignServer/Services/OdbDesignServiceImpl.h"
#include "OdbDesignServer/Config/GrpcServiceConfig.h"
#include <App/DesignCache.h>
#include <service.grpc.pb.h>

using namespace Odb::Lib::App;
using namespace Odb::Test::Fixtures;
using namespace OdbDesignServer::Services;
using namespace OdbDesignServer::Config;
using namespace Odb::Grpc;

namespace Odb::Test
{
	// =========================================================================
	// HealthCheck real-transport coverage (release-readiness U4).
	//
	// HealthCheck had NO test coverage at all (direct or over the wire).
	// These tests run a real loopback server + generated stub and assert the
	// wire behavior of the custom (non grpc.health.v1) health RPC.
	//
	// SEMANTICS FINDING (asserted as-implemented, flagged for a product
	// decision): the standard grpc.health.v1 protocol returns the
	// NOT_FOUND grpc status for a service name the server does not know.
	// OdbDesignService.HealthCheck instead returns OK with
	// HealthCheckResponse::NOT_SERVING for any non-empty name other than
	// "OdbDesignService". A generic grpc.health.v1 client (e.g. grpc-health-probe,
	// Kubernetes gRPC health probes) keys its decision on the response's
	// ServingStatus enum, NOT on the grpc status code, so the wire-visible
	// outcome for an unknown service (NOT_SERVING) still reads as "unhealthy"
	// — but the transport-level status differs from the standard protocol's
	// NOT_FOUND. Switching is a wire-contract change for any custom client
	// that checks status.ok(); until decided, these tests pin the actual
	// contract.
	// =========================================================================
	class HealthCheckTest : public FileArchiveLoadFixture
	{
	protected:
		void SetUp() override
		{
			if (getTestDataDir().empty())
			{
				GTEST_SKIP() << "Test data directory not available (ODB_TEST_DATA_DIR not set)";
				return;
			}
			FileArchiveLoadFixture::SetUp();

			m_sharedDesignCache = std::shared_ptr<DesignCache>(m_pDesignCache.release());
			m_config = GrpcServiceConfig::CreateDefault();
			m_service = std::make_unique<OdbDesignServiceImpl>(m_sharedDesignCache, m_config);

			grpc::ServerBuilder builder;
			int port = 0;
			builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &port);
			builder.RegisterService(m_service.get());
			m_server = builder.BuildAndStart();
			ASSERT_NE(m_server, nullptr);
			ASSERT_GT(port, 0);

			m_stub = OdbDesignService::NewStub(grpc::CreateChannel(
				"127.0.0.1:" + std::to_string(port), grpc::InsecureChannelCredentials()));
		}

		void TearDown() override
		{
			if (m_server != nullptr)
			{
				m_server->Shutdown();
				m_server = nullptr;
			}
			m_stub.reset();
			m_service.reset();

			FileArchiveLoadFixture::TearDown();
		}

		// Runs HealthCheck over the real channel and returns
		// (grpc status ok?, ServingStatus).
		std::pair<bool, HealthCheckResponse::ServingStatus> CheckHealth(const std::string& serviceName)
		{
			grpc::ClientContext ctx;
			HealthCheckRequest req;
			req.set_service(serviceName);
			HealthCheckResponse resp;
			const auto status = m_stub->HealthCheck(&ctx, req, &resp);
			return { status.ok(), resp.status() };
		}

		std::shared_ptr<DesignCache> m_sharedDesignCache;
		std::shared_ptr<GrpcServiceConfig> m_config;
		std::unique_ptr<OdbDesignServiceImpl> m_service;
		std::unique_ptr<grpc::Server> m_server;
		std::unique_ptr<OdbDesignService::Stub> m_stub;
	};

	TEST_F(HealthCheckTest, HealthCheck_EmptyServiceName_ReturnsServing)
	{
		// Per grpc.health.v1 convention an empty service name is the
		// server-overall health check; this server must answer SERVING.
		const auto [ok, servingStatus] = CheckHealth("");
		EXPECT_TRUE(ok);
		EXPECT_EQ(servingStatus, HealthCheckResponse::SERVING);
	}

	TEST_F(HealthCheckTest, HealthCheck_ExactServiceName_ReturnsServing)
	{
		const auto [ok, servingStatus] = CheckHealth("OdbDesignService");
		EXPECT_TRUE(ok);
		EXPECT_EQ(servingStatus, HealthCheckResponse::SERVING);
	}

	TEST_F(HealthCheckTest, HealthCheck_UnknownServiceName_ReturnsNotServingWithOkStatus)
	{
		// Pins the ACTUAL contract for an unknown non-empty service name:
		// grpc status OK + NOT_SERVING (NOT the grpc.health.v1 standard's
		// NOT_FOUND transport status — see the semantics finding above).
		const auto [ok, servingStatus] = CheckHealth("SomeOtherService");
		EXPECT_TRUE(ok);
		EXPECT_EQ(servingStatus, HealthCheckResponse::NOT_SERVING);
	}
}
