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
#include <filesystem>

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
		// Try config.json in current directory
		std::string configPath = "config.json";
		auto loadResult = OdbDesignServer::Config::GrpcServiceConfig::LoadFromFile(configPath);
		std::cout << loadResult.message << std::endl;

		OdbDesignServer::Services::OdbDesignServiceImpl service(cache, loadResult.config);

		grpc::EnableDefaultHealthCheckService(true);
		grpc::reflection::InitProtoReflectionServerBuilderPlugin();
		
		grpc::ServerBuilder builder;
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