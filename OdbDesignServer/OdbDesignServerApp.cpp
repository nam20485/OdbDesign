#include "OdbDesignServerApp.h"
#include "Controllers/HelloWorldController.h"
#include "Controllers/FileUploadController.h"
#include "Controllers/FileModelController.h"
#include "Controllers/HealthCheckController.h"
#include "Controllers/DesignsController.h"
#include "Services/OdbDesignServiceImpl.h"
#include "Config/GrpcServiceConfig.h"
#include "macros.h"
#include <App/BasicRequestAuthentication.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <iostream>
#include <utility>
#include <memory>
#include <string>
#include <App/DesignCache.h>
#include "OdbServerAppBase.h"
#include <crow/middlewares/cors.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/resource_quota.h>
#include <filesystem>
#include <cstdlib>
#include <cstring>
#include <cstdlib>
#include <cstring>

using namespace Odb::Lib::App;

namespace Odb::App::Server
{
	OdbDesignServerApp::OdbDesignServerApp(int argc, char* argv[])
		: OdbServerAppBase(argc, argv)
	{		
	}

	void OdbDesignServerApp::add_controllers()
	{
		m_vecControllers.push_back(std::make_shared<HelloWorldController>(*this));		
		m_vecControllers.push_back(std::make_shared<FileUploadController>(*this));
		m_vecControllers.push_back(std::make_shared<FileModelController>(*this));
		m_vecControllers.push_back(std::make_shared<HealthCheckController>(*this));
		m_vecControllers.push_back(std::make_shared<DesignsController>(*this));
	}

	void OdbDesignServerApp::RunGrpcServer(const std::string& server_address, std::shared_ptr<Odb::Lib::App::DesignCache> cache)
	{
		if (!cache)
		{
			std::cerr << "ERROR: DesignCache is null, cannot start gRPC server" << std::endl;
			return;
		}

		// Load gRPC service configuration
		// Check GRPC_CONFIG_PATH environment variable first, then try executable directory, then current directory
		std::string configPath;
		const char* envConfig = std::getenv("GRPC_CONFIG_PATH");
		if (envConfig != nullptr && strlen(envConfig) > 0)
		{
			configPath = envConfig;
		}
		else
		{
			// Try executable directory first (where config.json should be)
			std::filesystem::path exeDir = args().executableDirectory();
			std::filesystem::path configInExeDir = exeDir / "config.json";
			
			// Try current working directory as fallback
			std::filesystem::path configInCwd = "config.json";
			
			// Check executable directory first, then current directory
			if (std::filesystem::exists(configInExeDir))
			{
				configPath = configInExeDir.string();
			}
			else if (std::filesystem::exists(configInCwd))
			{
				configPath = configInCwd.string();
			}
			else
			{
				// Default to executable directory (will show "not found" message)
				configPath = configInExeDir.string();
			}
		}
		auto loadResult = OdbDesignServer::Config::GrpcServiceConfig::LoadFromFile(configPath);
		std::cout << loadResult.message << std::endl;

		OdbDesignServer::Services::OdbDesignServiceImpl service(cache, loadResult.config);

		grpc::EnableDefaultHealthCheckService(true);
		grpc::reflection::InitProtoReflectionServerBuilderPlugin();
		
		grpc::ServerBuilder builder;
		
		// Apply message size limits from configuration
		// Convert MB to bytes (multiply by 1024*1024)
		int maxReceiveBytes = loadResult.config->max_receive_message_size_mb * 1024 * 1024;
		int maxSendBytes = loadResult.config->max_send_message_size_mb * 1024 * 1024;
		builder.SetMaxReceiveMessageSize(maxReceiveBytes);
		builder.SetMaxSendMessageSize(maxSendBytes);

		std::cout << "gRPC max message sizes: receive=" << loadResult.config->max_receive_message_size_mb
				  << "MB, send=" << loadResult.config->max_send_message_size_mb << "MB" << std::endl;
	// Apply compression configuration (level-based — gRPC selects algorithm per peer)
	if (loadResult.config->compression_level != GRPC_COMPRESS_LEVEL_NONE)
	{
		builder.SetDefaultCompressionLevel(loadResult.config->compression_level);
		std::cout << "gRPC compression level: "
				  << OdbDesignServer::Config::GrpcServiceConfig::CompressionLevelToString(loadResult.config->compression_level)
				  << std::endl;
	}
	else
	{
		std::cout << "gRPC compression: disabled" << std::endl;
	}

		// Apply thread pool limits via ResourceQuota
		grpc::ResourceQuota quota;
		quota.SetMaxThreads(loadResult.config->max_threads);
		builder.SetResourceQuota(quota);
		builder.SetSyncServerOption(grpc::ServerBuilder::SyncServerOption::MIN_POLLERS,
		                            loadResult.config->min_pollers);

		std::cout << "gRPC thread pool: max_threads=" << loadResult.config->max_threads
				  << ", min_pollers=" << loadResult.config->min_pollers << std::endl;

		// Apply keepalive channel arguments
		builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS,
		                           loadResult.config->keepalive_time_s * 1000);
		builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS,
		                           loadResult.config->keepalive_timeout_s * 1000);
		builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS,
		                           loadResult.config->keepalive_permit_without_calls ? 1 : 0);
		builder.AddChannelArgument(GRPC_ARG_HTTP2_MIN_RECV_PING_INTERVAL_WITHOUT_DATA_MS,
		                           (loadResult.config->keepalive_time_s * 1000) / 2);

		std::cout << "gRPC keepalive: time=" << loadResult.config->keepalive_time_s
				  << "s, timeout=" << loadResult.config->keepalive_timeout_s
				  << "s, permit_without_calls="
				  << (loadResult.config->keepalive_permit_without_calls ? "true" : "false")
				  << std::endl;

		builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
		builder.RegisterService(&service);

		 // Build server and transfer ownership to base via shared_ptr
		std::unique_ptr<grpc::Server> serverUnique = builder.BuildAndStart();
		if (!serverUnique) {
			std::cerr << "ERROR: Failed to start gRPC server on " << server_address << std::endl;
			return;
		}
		std::cout << "gRPC server listening on " << server_address << std::endl;		

		  // Share ownership so base class can Shutdown() it from another thread
		std::shared_ptr<grpc::Server> serverShared(std::move(serverUnique));
		set_grpc_server(serverShared);

		// Wait for shutdown signal
		serverShared->Wait();
		std::cout << "gRPC server shut down." << std::endl;

		// Release ownership after shutdown (base will also reset on its side)
		set_grpc_server(nullptr);
	}

	bool OdbDesignServerApp::preServerRun()
	{
		// CORS
		auto& cors = crow_app().get_middleware<crow::CORSHandler>();
		if (Utils::IsProduction())
		{
			cors.global()
				.headers("*")
				.origin("*");
		}
		else
		{
			cors.global()
				.headers("*")
				.origin("*");
		}

		// add authentication
		bool disableAuth = args().disableAuthentication();
		auto basicRequestAuth = std::make_unique<BasicRequestAuthentication>(BasicRequestAuthentication(disableAuth));
		request_auth(std::move(basicRequestAuth));

		return true;
	}
}