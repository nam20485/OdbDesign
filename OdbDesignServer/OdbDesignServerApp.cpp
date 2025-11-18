#include "OdbDesignServerApp.h"
#include "Controllers/HelloWorldController.h"
#include "Controllers/FileUploadController.h"
#include "Controllers/FileModelController.h"
#include "Controllers/HealthCheckController.h"
#include "Controllers/DesignsController.h"
#include "Services/OdbDesignServiceImpl.h"
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
#include <App/OdbServerAppBase.h>
#include <crow/middlewares/cors.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

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

		OdbDesignServer::Services::OdbDesignServiceImpl service(cache);

		grpc::EnableDefaultHealthCheckService(true);
		grpc::reflection::InitProtoReflectionServerBuilderPlugin();
		
		grpc::ServerBuilder builder;
		builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
		builder.RegisterService(&service);

		std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
		std::cout << "gRPC server listening on " << server_address << std::endl;

		// Wait for shutdown signal
		server->Wait();
		std::cout << "gRPC server shut down." << std::endl;
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